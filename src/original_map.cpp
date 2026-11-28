#include "original_map.h"

namespace s2ne
{
auto OriginalMap::viewport_pixel(const std::vector<uint8_t>& source, int x, int y) const -> uint8_t
{
    const int sx = camera_x + x, sy = camera_y + y;
    if (sx < 0 || sy < 0 || sx >= int(width * 64) || sy >= int(height * 32)) { return 0; }
    return source.at(size_t(sy) * width * 64 + sx);
}

auto OriginalMap::overlay_bitmap(UnitBitmap& destination, const UnitBitmap& source, bool background) -> void
{
    const auto& pixels = background ? source.b1 : source.ad;
    if (pixels.size() != size_t(source.w) * source.h)
    {
        return;
    }
    for (unsigned y = 0; y < source.h; ++y)
    {
        for (unsigned x = 0; x < source.w; ++x)
        {
            int dx = source.left - destination.left + int(x), dy = source.top - destination.top + int(y);
            if (dx < 0 || dy < 0 || dx >= int(destination.w) || dy >= int(destination.h))
            {
                continue;
            }
            auto c = pixels[size_t(y) * source.w + x];
            if (background || c)
            {
                destination.b1[size_t(dy) * destination.w + unsigned(dx)] = c;
            }
        }
    }
}

auto OriginalMap::cell_object(size_t cell) const -> unsigned
{
    return std::find(cleared_object_cells_.begin(), cleared_object_cells_.end(), cell) != cleared_object_cells_.end() ? 0 : word(prg_, cell + 3);
}

auto OriginalMap::group(unsigned id) const -> size_t { return dword(pal_, 808 + 8 * (64 + id - 1)) - 10 - (id - 1); }

auto OriginalMap::try_item(unsigned archive, unsigned index) const -> Result<size_t>
{
    size_t g = group(archive);
    if (index >= word(prg_, g))
    {
        return Result<size_t>::failure(make_error("Invalid original map sprite index archive=" + std::to_string(archive) + " index=" + std::to_string(index) + " count=" + std::to_string(word(prg_, g))));
    }
    return Result<size_t>::success(g + dword(prg_, g + 2 + index * 8));
}

auto OriginalMap::item(unsigned archive, unsigned index) const -> size_t
{
    const auto offset = try_item(archive, index).value();
    if (archive >= 3 && archive <= 5) { texture_items_[offset] = { archive, index + 1 }; }
    return offset;
}

auto OriginalMap::try_raw(size_t offset) const -> Result<Image>
{
    if (cache_.contains(offset))
    {
        return Result<Image>::success(cache_.at(offset));
    }
    if (offset > prg_.size() || prg_.size() - offset < 4)
    {
        return Result<Image>::failure(make_error("Truncated original map sprite header"));
    }
    Image image{ word(prg_, offset) + 1, word(prg_, offset + 2) + 1, {} };
    if (image.w > 8192 || image.h > 8192)
    {
        return Result<Image>::failure(make_error("Invalid original map sprite dimensions"));
    }
    if (offset + 4 > prg_.size() || size_t(image.w) * image.h > prg_.size() - offset - 4)
    {
        return Result<Image>::failure(make_error("Truncated original map sprite"));
    }
    image.pixels.assign(prg_.begin() + offset + 4, prg_.begin() + offset + 4 + size_t(image.w) * image.h);
    if (auto found = texture_items_.find(offset); found != texture_items_.end())
    {
        image.texture_group = found->second.first;
        image.texture_id = found->second.second;
    }
    cache_.emplace(offset, image);
    return Result<Image>::success(std::move(image));
}

auto OriginalMap::raw(size_t offset) const -> const Image&
{
    if (cache_.contains(offset))
    {
        return cache_.at(offset);
    }
    auto result = try_raw(offset);
    if (!result)
    {
        fatal(result.error());
    }
    return cache_.at(offset);
}

auto OriginalMap::mask_unit_ground(std::vector<uint8_t>& pixels, unsigned w, unsigned h, int x, int y, int elevation, bool checker, int displacement) const -> void
{
    // 42AFBA -> 42F027 -> 42F2A7; 423D76 clears covered sprite pixels.
    const size_t cells = item(6, map_id - 1) + 28;
    const int left = x * 64 + (y & 1) * 32 + 32 - int(w / 2) + displacement, top = y * 16 + 32 - int(h);
    auto clear = [&](const Image& tile, int dx, int dy, bool object = false)
    {
        const int x0 = std::max(0, dx), y0 = std::max(0, dy);
        const int x1 = std::min(int(w), dx + int(tile.w)), y1 = std::min(int(h), dy + int(tile.h));
        for (int yy = y0; yy < y1; ++yy)
        {
            for (int xx = x0; xx < x1; ++xx)
            {
                unsigned c = tile.pixels.at(size_t(yy - dy) * tile.w + xx - dx);
                if (c && (!object || (c != 32 && c != 33)) && (!checker || ((xx - x0 + yy - y0) & 1)))
                {
                    pixels.at(size_t(yy) * w + xx) = 0;
                }
            }
        }
    };
    auto cell = [&](int cx, int cy, int ring)
    {
        if (cy < 1 || cy >= int(height * 2) || cx < 0 || (cy & 1 ? cx >= int(width) - 1 : cx == 0 || cx >= int(width)))
        {
            return;
        }
        size_t p = cells + (size_t(cy) * width + cx) * 5;
        unsigned ground = word(prg_, p + 1);
        int rise = int(prg_.at(p)) - elevation;
        if (ground <= 4 || !prg_.at(p))
        {
            return;
        }
        int levels = rise - ring * 2 + 1;
        const auto& tile = raw(item(3, ground - 1));
        int dx = cx * 64 + (cy & 1) * 32 - left, dy = cy * 16 - top;
        if (!levels)
        {
            clear(tile, dx, dy - rise * 16);
        }
        else if (levels > 0)
        {
            for (int z = -1; z < levels; ++z)
            {
                clear(tile, dx, dy - (ring * 2 + z) * 16);
            }
        }
        unsigned object = cell_object(p);
        if (!object || levels < -4)
        {
            return;
        }
        unsigned type = object_attributes_.at(object - 1) >> 8;
        if ((type == 12 && composing_static_ && !dynamic_object_chains_.empty()) || (type == 5 && !event_flags_[1]) || (type == 6 && !event_flags_[0]) || (type == 11 && !event_flags_[3]))
        {
            return;
        }
        clear(raw(item(4, object - 1)), dx, dy - rise * 16 - 64, true);
    };
    if (elevation <= 0)
    {
        return;
    }
    for (int ring = 1; ring <= 12; ++ring)
    {
        int row = y + ring * 2;
        cell(x, row, ring);
        cell(x - ((row - 1) & 1), row - 1, ring);
        cell(x + (((row - 1) & 1) ? 0 : 1), row - 1, ring);
        if ((w / 2 > 32 || displacement) && ring != 12)
        {
            cell(x - 1, row, ring);
            cell(x + 1, row, ring);
            // 42F1F3/42F24D extends the nearer row toward displaced frames.
            if (displacement < 0)
            {
                cell(x - 1 - ((row - 1) & 1), row - 1, ring);
            }
            else if (displacement > 0)
            {
                cell(x + (((row - 1) & 1) ? 1 : 2), row - 1, ring);
            }
        }
    }
    if (word(prg_, cells + (size_t(y) * width + x) * 5 + 1) <= 4)
    {
        return;
    }
    // 42F4D7 uses scenery bounds and its attribute depth, not row sorting.
    size_t header = item(6, map_id - 1), scenery = cells + size_t(width) * height * 10;
    size_t special = scenery + word(prg_, header + 12) * 10;
    for (unsigned i = 0; i < word(prg_, header + 14); ++i)
    {
        size_t p = special + i * 6;
        if (int(int16_t(word(prg_, p))) == x && int(int16_t(word(prg_, p + 2))) == y)
        {
            if (word(prg_, p + 4) & 0x4000)
            {
                return;
            }
            break;
        }
    }
    int unit_top = top - (elevation - 1) * 16;
    for (unsigned i = 0; i < word(prg_, header + 12); ++i)
    {
        size_t p = scenery + i * 10;
        unsigned id = word(prg_, p + 8);
        int sx = int(int16_t(word(prg_, p))), sy = int(int16_t(word(prg_, p + 2)));
        int sw = int(int16_t(word(prg_, p + 4))), sh = int(int16_t(word(prg_, p + 6))), depth = scenery_attributes_.at(id - 1);
        if (depth && sy + sh / 16 - depth <= y)
        {
            continue;
        }
        const auto& image = raw(item(5, id - 1));
        int bx = sx + sw / 128, by = sy + sh / 16 - 2, scene_elevation = 0;
        if (bx >= 0 && bx < int(width) && by >= 1 && by < int(height * 2) && ((by & 1) || (bx != 0 && bx != int(width) - 1)))
        {
            scene_elevation = prg_.at(cells + (size_t(by) * width + bx) * 5);
        }
        int scene_left = sx * 64 + (sy & 1) * 32, scene_top = sy * 16 - std::max(0, scene_elevation - 1) * 16;
        if (scene_top + sh <= unit_top || unit_top + int(h) <= scene_top || scene_left + sw <= left || left + int(w) <= scene_left)
        {
            continue;
        }
        clear(image, scene_left - left, scene_top - unit_top, true);
    }
}

auto OriginalMap::try_backdrop(unsigned id) const -> Result<Image>
{
    ResourceReader reader(prg_);
    size_t g = group(8);
    unsigned count = reader.number(g);
    if (!id || id > count)
    {
        return Result<Image>::failure(make_error("Invalid backdrop ID"));
    }
    size_t entry = g + 2 + size_t(id - 1) * 8;
    size_t offset = reader.number(entry, 4), length = reader.number(entry + 4, 4);
    if (!reader)
    {
        return Result<Image>::failure(reader.error());
    }
    if (g > prg_.size() || offset > prg_.size() - g || length < 5 || !reader.extent(g + offset, length))
    {
        return Result<Image>::failure(make_error("Invalid backdrop extent"));
    }
    size_t p = g + offset, end = p + length;
    unsigned encoding = prg_.at(p++);
    Image image{ word(prg_, p) + 1, word(prg_, p + 2) + 1, {} };
    image.texture_group = 8;
    image.texture_id = id;
    p += 4;
    if (image.w > 8192 || image.h > 8192 || end > prg_.size())
    {
        return Result<Image>::failure(make_error("Invalid original map backdrop"));
    }
    std::vector<uint8_t> pixels;
    pixels.reserve(size_t(image.w) * image.h);
    std::vector<uint8_t> literal(size_t(image.w) * image.h, 0);
    if (encoding)
    {
        if (size_t(image.w) * image.h > end - p)
        {
            return Result<Image>::failure(make_error("Truncated original raw backdrop"));
        }
        pixels.assign(prg_.begin() + p, prg_.begin() + p + size_t(image.w) * image.h);
    }
    else
    {
        for (unsigned y = 0; y < image.h; ++y)
        {
            for (unsigned x = 0; x < image.w;)
            {
                if (p >= end)
                {
                    return Result<Image>::failure(make_error("Truncated original backdrop run"));
                }
                unsigned code = prg_.at(p++), n = code & 127;
                if (!code)
                {
                    pixels.push_back(0);
                    ++x;
                    continue;
                }
                if (!n || n > image.w - x)
                {
                    return Result<Image>::failure(make_error("Invalid original backdrop run"));
                }
                unsigned c = 0;
                if (!(code & 128))
                {
                    if (p >= end)
                    {
                        return Result<Image>::failure(make_error("Truncated original backdrop repeat"));
                    }
                    c = prg_.at(p++);
                }
                while (n--)
                {
                    if (code & 128)
                    {
                        if (p >= end)
                        {
                            return Result<Image>::failure(make_error("Truncated original backdrop literal"));
                        }
                        c = prg_.at(p++);
                        literal.at(pixels.size()) = 1;
                    }
                    pixels.push_back(uint8_t(c));
                    ++x;
                }
            }
        }
    }
    image.pixels = SharedBytes(std::move(pixels));
    image.literal = SharedBytes(std::move(literal));
    return Result<Image>::success(std::move(image));
}

auto OriginalMap::backdrop(unsigned id) const -> Image
{
    auto result = try_backdrop(id);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto OriginalMap::blit(const Image& image, int x, int y, bool transparent, bool object) -> void
{
    if (texture_resources.active()) { texture_placements_.push_back({ image, x, y, transparent }); }
    for (unsigned row = 0; row < image.h; ++row)
    {
        for (unsigned col = 0; col < image.w; ++col)
        {
            int dx = x + int(col), dy = y + int(row);
            if (dx < 0 || dy < 0 || dx >= int(width * 64) || dy >= int(height * 32))
            {
                continue;
            }
            unsigned c = image.pixels.at(size_t(row) * image.w + col);
            if (transparent && !c && (image.literal.empty() || !image.literal.at(size_t(row) * image.w + col)))
            {
                continue;
            }
            auto& pixel = pixels_.at(size_t(dy) * width * 64 + dx);
            pixel = object && (c == 32 || c == 33) ? object_shadows_[c - 32][pixel] : uint8_t(c);
        }
    }
}

auto OriginalMap::visible_exit_cells() const -> std::vector<std::pair<int, int>>
{
    std::vector<std::pair<int, int>> cells;
    if (!event_flags_[0]) { return cells; }
    for (unsigned y = 0; y < height * 2; ++y)
    {
        for (unsigned x = 0; x < width - (y & 1); ++x)
        {
            unsigned object = object_at(int(x), int(y));
            if (object && (object_attributes_.at(object - 1) >> 8) == 6) { cells.emplace_back(int(x), int(y)); }
        }
    }
    return cells;
}

auto OriginalMap::remove_dynamic_object(int x, int y) -> bool
{
    auto found = std::find_if(dynamic_objects_.begin(), dynamic_objects_.end(), [&](const auto& object)
        {
            return object.x == x && object.y == y;
        });
    if (found == dynamic_objects_.end())
    {
        return false;
    }
    if (texture_resources.active())
    {
        const int left = found->left, top = found->top;
        std::erase_if(texture_placements_, [&](const auto& placement)
            {
                return placement.image.texture_group == 4 && placement.x == left && placement.y == top;
            });
    }
    blit(found->background, found->left, found->top, false);
    cleared_object_cells_.push_back(item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5);
    dynamic_objects_.erase(found);
    removed_dynamic_cells_.emplace_back(x, y);
    return true;
}

auto OriginalMap::try_object_at(int x, int y) const -> Result<unsigned>
{
    if (x < 0 || y < 0 || unsigned(y) >= height * 2 || unsigned(x) >= width - (unsigned(y) & 1))
    {
        return Result<unsigned>::failure(make_error("Invalid original object cell"));
    }
    return Result<unsigned>::success(cell_object(item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5));
}

auto OriginalMap::object_at(int x, int y) const -> unsigned
{
    auto result = try_object_at(x, y);
    if (!result)
    {
        report_error(result.error());
        return 0;
    }
    return result.value();
}

auto OriginalMap::dynamic_marker(int x, int y) const -> unsigned
{
    size_t m = item(6, map_id - 1), special = m + 28 + size_t(width) * height * 2 * 5 + word(prg_, m + 12) * 10;
    for (unsigned i = 0; i < word(prg_, m + 14); ++i)
    {
        size_t p = special + i * 6;
        if (int(int16_t(word(prg_, p))) == x && int(int16_t(word(prg_, p + 2))) == y)
        {
            return word(prg_, p + 4);
        }
    }
    return 0;
}

auto OriginalMap::mask_selection(std::vector<uint8_t>& pixels, int x, int y) const -> void
{
    if (pixels.empty() || pixels.size() % 64 || x < 0 || y < 0 || unsigned(y) >= height * 2 || unsigned(x) >= width - (unsigned(y) & 1)) { return; }
    const auto elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5);
    // 42AFBA -> 42F027: the range/cursor bitmap uses the same forward clipping rings.
    mask_unit_ground(pixels, 64, unsigned(pixels.size() / 64), x, y, elevation);
}

auto OriginalMap::try_mask_selection_units(std::vector<uint8_t>& pixels, int x, int y, const std::vector<OriginalEvents::Unit>& units, unsigned turn, uint64_t elapsed, int layer, bool idle_cursor) const -> Result<bool>
{
    if (pixels.size() != 64 * 137) { return Result<bool>::success(true); }
    if (!map_id || x < 0 || y < 0 || unsigned(y) >= height * 2 || unsigned(x) >= width - (unsigned(y) & 1)) { return Result<bool>::failure(make_error("Invalid selection unit mask cell")); }
    const size_t cells = item(6, map_id - 1) + 28;
    const int elevation = prg_.at(cells + (size_t(y) * width + x) * 5);
    const int left = x * 64 + (y & 1) * 32, top = y * 16 + 32 - 137;
    auto output = pixels;
    bool enemy_context = false;
    for (const auto& unit : units)
    {
        enemy_context = enemy_context || unit.behavior == 2 || unit.behavior == 3 || unit.behavior == 8;
        if (!unit.present(turn) || (unit.status_flags & 0x20) || !(unit.status_flags & 0x80) || unit.action_state == 3 || unit.action_state == 5 || (layer == 1 && unit.action_state == 1)) { continue; }
        const bool same = unit.x == x && unit.y == y;
        // 431347: rear uses mode3/include-current; front uses mode1,
        // with mode2 for the friendly roster and mode0 for the enemy roster.
        const bool include_same = layer == 0 || (idle_cursor && !enemy_context);
        if (same && !include_same) { continue; }
        if (std::abs(unit.x - x) > 2 || unit.y > y + 24 || (layer == 0 ? unit.y < y : (unit.y <= y && !(same && include_same)))) { continue; }
        if (!unit.appearance || unit.x < 0 || unit.y < 0 || unsigned(unit.y) >= height * 2 || unsigned(unit.x) >= width - (unsigned(unit.y) & 1)) { return Result<bool>::failure(make_error("Invalid selection unit mask source")); }
        const int other_elevation = prg_.at(cells + (size_t(unit.y) * width + unit.x) * 5);
        if (!other_elevation) { continue; }
        auto frame = try_idle_frame(unit.appearance, unit.movement_pose ? uint64_t(unit.movement_frame) * 190 : idle_pose_time_.value_or(elapsed), unit.direction, unit.animation_action);
        if (!frame) { return Result<bool>::failure(frame.error()); }
        const size_t index = group(1), frames = group(2);
        const unsigned first = word(prg_, index + 2 + (unit.appearance - 1) * 8 + 2);
        size_t entry = frames + 2 + (first + frame.value() - 1) * 7;
        size_t p = frames + dword(prg_, entry), length = word(prg_, entry + 4);
        if (length < 4 || p > prg_.size() || length > prg_.size() - p) { return Result<bool>::failure(make_error("Invalid selection unit mask entry")); }
        size_t end = p + length;
        unsigned w = word(prg_, p) + 1, h = word(prg_, p + 2) + 1;
        if (w > 512 || h > 512) { return Result<bool>::failure(make_error("Invalid selection unit mask dimensions")); }
        p += 4;
        size_t offset = 0;
        const int dx = unit.x * 64 + (unit.y & 1) * 32 + 32 - int(w / 2) - left;
        const int dy = unit.y * 16 + 32 - int(h) - top - (other_elevation - elevation) * 16;
        while (p < end)
        {
            unsigned code = prg_.at(p++), count = code ? code & 127 : 1;
            if (count > size_t(w) * h - offset) { return Result<bool>::failure(make_error("Invalid selection unit mask run")); }
            unsigned source = 0;
            if (code & 128) { source = count; }
            else if (code) { source = 1; }

            if (source > end - p) { return Result<bool>::failure(make_error("Truncated selection unit mask run")); }
            unsigned repeated = code && !(code & 128) ? prg_.at(p++) : 0;
            while (count--)
            {
                unsigned ink = code & 128 ? prg_.at(p++) : repeated;
                int px = dx + int(offset % w), py = dy + int(offset / w);
                if (ink && px >= 0 && px < 64 && py >= 0 && py < 137) { output[size_t(py) * 64 + px] = 0; }
                ++offset;
            }
        }
        if (offset != size_t(w) * h) { return Result<bool>::failure(make_error("Truncated selection unit mask")); }
    }
    pixels.swap(output);
    return Result<bool>::success(true);
}

auto OriginalMap::set_idle_pose_time(uint64_t time) -> void { idle_pose_time_ = time; }

auto OriginalMap::set_checker_mask(bool enabled) -> void { checker_mask_ = enabled; }

auto OriginalMap::set_dynamic_object_chains(const std::vector<std::array<unsigned, 4>>& chains) -> void { dynamic_object_chains_ = chains; }

auto OriginalMap::set_event_flags(const std::array<uint8_t, 80>& flags) -> void
{
    bool changed = event_flags_[0] != flags[0] || event_flags_[1] != flags[1] || event_flags_[3] != flags[3];
    event_flags_ = flags;
    if (changed && map_id)
    {
        int x = camera_x, y = camera_y;
        auto removed = removed_dynamic_cells_;
        load(map_id);
        for (auto [cx, cy] : removed)
        {
            remove_dynamic_object(cx, cy);
        }
        camera_x = x;
        camera_y = y;
    }
}

auto OriginalMap::draw_terrain_validated(const OriginalMenu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const -> Result<bool>
{
    if (!map_id || x < 0 || y < 0 || x >= int(width) || y >= int(height * 2)) { return Result<bool>::success(true); }
    size_t m = item(6, map_id - 1), cell = m + 28 + (size_t(y) * width + x) * 5;
    unsigned ground = word(prg_, cell + 1), object = cell_object(cell);
    if (ground == 1) { return Result<bool>::success(true); }
    size_t attributes = group(7), ground_attributes = attributes + 6;
    unsigned attr = ground ? word(prg_, ground_attributes + (ground - 1) * 2) : 0;
    if (!attr)
    {
        font.draw_original_large(image, font.text_bytes(0), 543, panel_y + 50, 179);
        return Result<bool>::success(true);
    }
    auto put = [&](unsigned ink, int dx, int dy)
    {
        if (!ink || dx < 0 || dx >= 640 || dy < 0 || dy >= 480) { return; }
        size_t p = (size_t(dy) * 640 + dx) * 4;
        for (unsigned c = 0; c < 3; ++c) { image.at(p + c) = uint8_t(unsigned(pal_.at(ink * 3 + c)) * 255 / 63); }
        image.at(p + 3) = 255;
    };
    const auto& ground_image = raw(item(3, ground - 1));
    bool ground_drawn = false;
    if (texture_resources.active())
    {
        auto available = texture_resources.group_available("ground");
        if (!available)
        {
            return Result<bool>::failure(available.error());
        }
        if (available.value())
        {
            auto result = texture_resources.draw_map_sprite(image, "ground", ground_image.texture_id, 526, panel_y + 40);
            if (!result)
            {
                return Result<bool>::failure(result.error());
            }
            ground_drawn = true;
        }
    }
    if (!ground_drawn)
    {
        for (unsigned yy = 0; yy < ground_image.h; ++yy)
        {
            for (unsigned xx = 0; xx < ground_image.w; ++xx)
            {
                unsigned ink = ground_image.pixels.at(size_t(yy) * ground_image.w + xx);
                if ((ground == 5 || ground == 6) && !ink) { continue; }
                put(ink, 526 + int(xx), panel_y + 40 + int(yy));
            }
        }
    }
    unsigned category = object ? object_attributes_.at(object - 1) >> 8 : 0;
    bool visible_object = object && !(category == 5 && !event_flags_[1]) && !(category == 6 && !event_flags_[0]) && !(category == 11 && !event_flags_[3]);
    if (visible_object)
    {
        const auto& object_image = raw(item(4, object - 1));
        bool object_drawn = false;
        if (texture_resources.active())
        {
            auto available = texture_resources.group_available("objects");
            if (!available)
            {
                return Result<bool>::failure(available.error());
            }
            if (available.value())
            {
                auto result = texture_resources.draw_map_sprite(image, "objects", object_image.texture_id, 526, panel_y - 56);
                if (!result)
                {
                    return Result<bool>::failure(result.error());
                }
                object_drawn = true;
            }
        }
        if (!object_drawn)
        {
            // 42C16F copies only the bottom 64x32 of the original 64x96 object image.
            for (unsigned yy = 0; yy < 32; ++yy)
            {
                for (unsigned xx = 0; xx < 64; ++xx)
                {
                    put(object_image.pixels.at(4096 + yy * 64 + xx), 526 + int(xx), panel_y + 40 + int(yy));
                }
            }
        }
    }
    auto small = [&](const std::string& text, int dx, int dy)
    {
        return font.try_draw_small_validated(image, { text.begin(), text.end() }, dx, panel_y + dy, 137, true);
    };
    auto coordinate = small("X=" + std::to_string(x), 595, 48);
    if (!coordinate) { return coordinate; }
    coordinate = small("Y=" + std::to_string(y), 595, 64);
    if (!coordinate) { return coordinate; }
    font.draw_original_large(image, [&]
        {
            auto value = std::to_string(prg_.at(cell));
            return std::vector<uint8_t>(value.begin(), value.end());
        }(),
        531, panel_y + 14, 141);
    unsigned local = attr & 255;
    if (visible_object && category != 7 && category != 12)
    {
        unsigned object_local = object_attributes_.at(object - 1) & 255;
        if (object_local <= terrain_object_names_) { local = terrain_ground_names_ + object_local; }
    }
    if (!local || size_t(local) * 11 > terrain_names_.size()) { return Result<bool>::success(true); }
    size_t start = size_t(local - 1) * 11;
    std::vector<uint8_t> name;
    for (size_t p = start; p < start + 11 && terrain_names_[p]; ++p) { name.push_back(terrain_names_[p]); }
    auto label = font.try_draw_small_validated(image, name, 596 - int(name.size()) * 4, panel_y + 17, 54, true);
    if (!label) { return label; }
    label = font.try_draw_small_validated(image, name, 595 - int(name.size()) * 4, panel_y + 16, 178, true);
    if (!label) { return label; }
    return Result<bool>::success(true);
}

auto OriginalMap::try_draw_terrain(const OriginalMenu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const -> Result<bool>
{
    if (image.size() != 640u * 480 * 4 || (panel_y != 0 && panel_y != 363))
    {
        return Result<bool>::failure(make_error("Invalid terrain framebuffer or panel position"));
    }
    bool extended_ground = false;
    if (map_id && x >= 0 && y >= 0 && x < int(width) && y < int(height * 2))
    {
        const size_t cell = item(6, map_id - 1) + 28 + (size_t(y) * width + x) * 5;
        const unsigned ground = word(prg_, cell + 1);
        if (ground > 1) { extended_ground = raw(item(3, ground - 1)).h > 77; }
    }
    if (texture_resources.active() || extended_ground)
    {
        auto output = image;
        auto result = draw_terrain_validated(font, output, x, y, panel_y);
        if (result) { image.swap(output); }
        return result;
    }
    const size_t first = size_t(std::max(0, panel_y - 56)) * 640 * 4;
    const size_t last = size_t(std::min(480, panel_y + 117)) * 640 * 4;
    std::vector<uint8_t> previous(image.begin() + first, image.begin() + last);
    auto result = draw_terrain_validated(font, image, x, y, panel_y);
    if (!result) { std::copy(previous.begin(), previous.end(), image.begin() + first); }
    return result;
}

auto OriginalMap::draw_terrain(const OriginalMenu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const -> void
{
    auto result = try_draw_terrain(font, image, x, y, panel_y);
    if (!result) { fatal(result.error()); }
}

auto OriginalMap::movement_effect_presented() -> void
{
    for (const auto& effect : pending_effect_presentations_)
    {
        movement_effect_clock_.presented(effect.camera, effect.time, movement_effect_stamps_[effect.actor]);
    }
    pending_effect_presentations_.clear();
}

auto OriginalMap::movement_effect_phase() const -> unsigned { return movement_effect_clock_.phase(); }

auto OriginalMap::set_army_modes(const std::optional<std::array<uint32_t, 2>>& modes) const -> void { army_modes_ = modes.value_or(std::array<uint32_t, 2>{}); }

auto OriginalMap::try_movement_effect(unsigned movement_class, unsigned frame, unsigned direction) const -> Result<std::vector<uint8_t>>
{
    if (movement_class != 4 && movement_class != 5)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("45CADA effect requires movement class4/5"));
    }
    auto resources = OriginalDiscResources(root_).try_dat_entry(disc_, 5);
    if (!resources) { return Result<std::vector<uint8_t>>::failure(resources.error()); }
    const auto& data = resources.value();
    ResourceReader reader(data);
    // 45CA1D's 4EAD9C table contains shared slots23/24; 45CAA0
    // indexes 4F70FC = 4F7060 +39*4 by direction&3.
    unsigned slot = movement_class == 4 ? 23 + (frame & 1) : 39 + (direction & 3);
    unsigned count = reader.number(0);
    if (slot > count) { return Result<std::vector<uint8_t>>::failure(make_error("Missing45CADA shared slot")); }
    size_t entry = 2 + size_t(slot - 1) * 8;
    size_t offset = reader.number(entry, 4), length = reader.number(entry + 4, 4);
    unsigned w = reader.number(offset) + 1, h = reader.number(offset + 2) + 1;
    if (!reader || w > 512 || h > 512 || length != 4 + size_t(w) * h || offset > data.size() || length > data.size() - offset)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid45CADA raw shared bitmap"));
    }
    return Result<std::vector<uint8_t>>::success({ data.begin() + offset, data.begin() + offset + length });
}

auto OriginalMap::try_movement_shadow(int x, int y) const -> Result<std::vector<uint8_t>>
{
    if (x < 0 || y < 0 || unsigned(x) >= width || unsigned(y) >= height * 2)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("448B99 map coordinate out of range"));
    }
    std::vector<uint8_t> background(2048);
    int elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(y) * width + x) * 5);
    int left = x * 64 + (y & 1) * 32, top = y * 16 - (elevation - 1) * 16;
    for (unsigned row = 0; row < 32; ++row)
    {
        for (unsigned column = 0; column < 64; ++column)
        {
            int sx = left + int(column), sy = top + int(row);
            if (sx >= 0 && sy >= 0 && sx < int(width * 64) && sy < int(height * 32))
            {
                background[size_t(row) * 64 + column] = pixels_[size_t(sy) * width * 64 + sx];
            }
        }
    }
    if (reaction_foot_mask_.empty())
    {
        auto resources = try_compose_reaction(64, 32, background, background, true);
        if (!resources)
        {
            return Result<std::vector<uint8_t>>::failure(resources.error());
        }
    }
    return s2ne::try_shadow(background, reaction_foot_mask_, reaction_shadow_);
}

auto OriginalMap::try_compose_reaction(unsigned w, unsigned h, std::span<const uint8_t> current, std::span<const uint8_t> background, bool left) const -> Result<std::vector<uint8_t>>
{
    if (reaction_foot_mask_.empty())
    {
        auto loaded = OriginalDiscResources(root_).try_dat_entry(disc_, 5);
        if (!loaded) { return Result<std::vector<uint8_t>>::failure(loaded.error()); }
        auto dat = std::move(loaded).value();
        size_t base = 0;
        size_t entry = base + 2 + 21 * 8, offset = base + dword(dat, entry);
        if (dword(dat, entry + 4) != 2052 || word(dat, offset) != 63 || word(dat, offset + 2) != 31 || offset + 2052 > dat.size())
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid original reaction foot mask"));
        }
        reaction_foot_mask_.assign(dat.begin() + offset + 4, dat.begin() + offset + 2052);
        const auto& palette = *palette_;
        for (unsigned i = 0; i < 256; ++i)
        {
            reaction_shadow_[i] = palette.map(5, i);
        }
    }
    return s2ne::try_reaction_bitmap(w, h, current, background, reaction_foot_mask_, reaction_shadow_, left);
}

auto OriginalMap::compose_reaction(unsigned w, unsigned h, std::span<const uint8_t> current, std::span<const uint8_t> background, bool left) const -> std::vector<uint8_t>
{
    auto result = try_compose_reaction(w, h, current, background, left);
    if (!result)
    {
        report_error(result.error());
        return {};
    }
    return std::move(result).value();
}

auto OriginalMap::set_resource_domain(OriginalDisc disc) -> void
{
    if (disc_ != disc)
    {
        disc_ = disc;
        reaction_foot_mask_.clear();
    }
}

auto OriginalMap::create(const std::filesystem::path& root, OriginalDisc disc) -> Result<OriginalMap>
{
    OriginalMap result;
    result.root_ = root;
    result.disc_ = disc;
    auto prg = read_shared_resource(root / "SUPERSP.PRG");
    auto pal = read_shared_resource(root / "SUPERSP.PAL");
    if (!prg)
    {
        return Result<OriginalMap>::failure(prg.error());
    }
    if (!pal)
    {
        return Result<OriginalMap>::failure(pal.error());
    }
    result.prg_ = std::move(prg).value();
    result.pal_ = std::move(pal).value();
    if (result.pal_.size() < 768)
    {
        return Result<OriginalMap>::failure(make_error("Truncated map palette"));
    }
    result.pal_.mutable_at(0) = result.pal_.mutable_at(1) = result.pal_.mutable_at(2) = 0;
    ResourceReader p(result.pal_), r(result.prg_);
    for (unsigned group = 1; group <= 8; ++group)
    {
        unsigned raw = p.number(808 + 8 * (64 + group - 1), 4);
        if (!p)
        {
            return Result<OriginalMap>::failure(p.error());
        }
        if (raw < 9 + group)
        {
            return Result<OriginalMap>::failure(make_error("Invalid map archive group"));
        }
        size_t base = raw - 9 - group;
        unsigned count = r.number(base);
        if (!r)
        {
            return Result<OriginalMap>::failure(r.error());
        }
        if (group == 7)
        {
            if (!r.extent(base, 6))
            {
                return Result<OriginalMap>::failure(r.error());
            }
            size_t size = r.number(base) + r.number(base + 2) + r.number(base + 4);
            if (!r.extent(base + 6, size * 2))
            {
                return Result<OriginalMap>::failure(r.error());
            }
            continue;
        }
        if (group == 1)
        {
            if (!r.extent(base + 2, size_t(count) * 208))
            {
                return Result<OriginalMap>::failure(r.error());
            }
            continue;
        }
        if (group == 2)
        {
            continue;
        }
        if (!r.extent(base + 2, size_t(count) * 8))
        {
            return Result<OriginalMap>::failure(r.error());
        }
        for (unsigned i = 0; i < count; ++i)
        {
            size_t offset = r.number(base + 2 + i * 8, 4), length = r.number(base + 6 + i * 8, 4);
            if (base > result.prg_.size() || offset > result.prg_.size() - base || !r.extent(base + offset, length))
            {
                return Result<OriginalMap>::failure(make_error("Invalid map archive entry"));
            }
            if (group >= 3 && group <= 5)
            {
                if (length < 4)
                {
                    return Result<OriginalMap>::failure(make_error("Truncated map image"));
                }
                size_t width = r.number(base + offset) + 1, height = r.number(base + offset + 2) + 1;
                if (width > 8192 || height > 8192 || width * height > length - 4)
                {
                    return Result<OriginalMap>::failure(make_error("Invalid map raw image"));
                }
            }
        }
    }
    unsigned raw = p.number(808 + 8 * 64, 4);
    size_t base = raw - 10;
    unsigned appearances = r.number(base);
    if (!r.extent(base + 2, size_t(appearances) * 208))
    {
        return Result<OriginalMap>::failure(r.error());
    }
    // 427F25: DAT6 names follow appearance_count * terrain_stride movement triples.
    auto movement = OriginalDiscResources(root).try_dat_entry(disc, 6);
    auto kinds = read_resource(root / "KINDSP.BIN");
    if (!movement || !kinds)
    {
        return Result<OriginalMap>::failure(!movement ? movement.error() : kinds.error());
    }
    const auto& table = movement.value();
    ResourceReader terrain_reader(table), kind_reader(kinds.value());
    unsigned stride = terrain_reader.number(0);
    result.terrain_ground_names_ = terrain_reader.number(2);
    result.terrain_object_names_ = terrain_reader.number(4);
    size_t names = 6 + size_t(stride) * kind_reader.number(22) * 3;
    size_t names_size = size_t(stride) * 11;
    if (!terrain_reader.extent(names, names_size) || !kind_reader)
    {
        return Result<OriginalMap>::failure(make_error("Invalid original terrain names"));
    }
    result.terrain_names_.assign(table.begin() + names, table.begin() + names + names_size);
    result.palette_ = std::make_shared<const OriginalPalette>(result.pal_);
    const auto& palette = *result.palette_;
    for (unsigned color = 0; color < 256; ++color)
    {
        result.object_shadows_[0][color] = palette.map(6, color);
        result.object_shadows_[1][color] = palette.map(4, color);
    }
    size_t attributes = result.group(7), objects = attributes + 6 + word(result.prg_, attributes) * 2;
    for (unsigned i = 0; i < word(result.prg_, attributes + 2); ++i)
    {
        result.object_attributes_.push_back(word(result.prg_, objects + i * 2));
    }
    size_t scenery = objects + word(result.prg_, attributes + 2) * 2;
    for (unsigned i = 0; i < word(result.prg_, attributes + 4); ++i)
    {
        result.scenery_attributes_.push_back(word(result.prg_, scenery + i * 2));
    }
    unsigned backgrounds = word(result.prg_, result.group(8));
    for (unsigned id = 1; id <= backgrounds; ++id)
    {
        auto image = result.try_backdrop(id);
        if (!image)
        {
            return Result<OriginalMap>::failure(image.error());
        }
    }
    return Result<OriginalMap>::success(std::move(result));
}

OriginalMap::OriginalMap(const std::filesystem::path& root, OriginalDisc disc)
{
    auto result = create(root, disc);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto OriginalMap::try_load(unsigned id) -> Result<bool>
{
    ResourceReader r(prg_);
    size_t g = group(6);
    unsigned count = r.number(g);
    if (!id || id > count)
    {
        return Result<bool>::failure(make_error("Invalid original map ID"));
    }
    size_t entry = g + 2 + size_t(id - 1) * 8, offset = r.number(entry, 4), length = r.number(entry + 4, 4);
    if (!r)
    {
        return Result<bool>::failure(r.error());
    }
    if (g > prg_.size() || offset > prg_.size() - g || length < 28 || !r.extent(g + offset, length))
    {
        return Result<bool>::failure(make_error("Invalid map record extent"));
    }
    size_t m = g + offset, end = m + length;
    unsigned w = r.number(m), h = r.number(m + 2);
    if (w < 10 || h < 15 || w > 256 || h > 256 || r.number(m + 4) != w * h * 2)
    {
        return Result<bool>::failure(make_error("Invalid original staggered map geometry"));
    }
    size_t cells = m + 28, scenery = cells + size_t(w) * h * 10, lookups = scenery + size_t(r.number(m + 12)) * 10 + size_t(r.number(m + 14)) * 6;
    if (lookups > end)
    {
        return Result<bool>::failure(make_error("Truncated map cells or scenery"));
    }
    std::array<std::vector<unsigned>, 3> mappings;
    for (unsigned layer = 0; layer < 3; ++layer)
    {
        unsigned images = word(prg_, group(3 + layer));
        if (lookups > end || end - lookups < 2)
        {
            return Result<bool>::failure(make_error("Truncated map lookup header"));
        }
        unsigned used = r.number(lookups);
        lookups += 2;
        if (size_t(images) * 2 + size_t(used) * 8 > end - lookups)
        {
            return Result<bool>::failure(make_error("Truncated map lookup table"));
        }
        for (unsigned i = 0; i < images; ++i)
        {
            unsigned value = r.number(lookups + i * 2);
            if (value > images)
            {
                return Result<bool>::failure(make_error("Invalid map image mapping"));
            }
            mappings[layer].push_back(value);
        }
        lookups += size_t(images) * 2 + size_t(used) * 8;
    }
    for (size_t i = 0; i < size_t(w) * h * 2; ++i)
    {
        unsigned ground = r.number(cells + i * 5 + 1), object = r.number(cells + i * 5 + 3);
        if ((ground && (ground > mappings[0].size() || !mappings[0][ground - 1])) || (object && (object > mappings[1].size() || !mappings[1][object - 1] || object > object_attributes_.size())))
        {
            return Result<bool>::failure(make_error("Invalid map tile reference"));
        }
    }
    for (unsigned i = 0; i < word(prg_, m + 12); ++i)
    {
        unsigned image = r.number(scenery + i * 10 + 8);
        if (!image || image > word(prg_, group(5)) || image > scenery_attributes_.size())
        {
            return Result<bool>::failure(make_error("Invalid map scenery reference"));
        }
    }
    for (unsigned field : { 16u, 22u })
    {
        unsigned background = r.number(m + field);
        if (background)
        {
            auto image = try_backdrop(background);
            if (!image)
            {
                return Result<bool>::failure(image.error());
            }
        }
    }
    OriginalMap candidate = *this;
    auto composed = candidate.load_validated(id);
    if (!composed)
    {
        return Result<bool>::failure(composed.error());
    }
    *this = std::move(candidate);
    return Result<bool>::success(true);
}

auto OriginalMap::load(unsigned id) -> void
{
    auto result = try_load(id);
    if (!result)
    {
        fatal(result.error());
    }
}

auto OriginalMap::load_validated(unsigned id) -> Result<bool>
{
    arrival_target_.reset();
    arrival_frame_ = arrival_elapsed_ = 0;
    arrival_presented_ = false;
    arrival_pose_time_ = 0;
    summon_target_.reset();
    summon_rows_ = summon_full_rows_ = 0;
    summon_presented_ = false;
    invisibility_world_.clear();
    for (auto& viewport : invisibility_viewports_) { viewport.clear(); }
    invisibility_scratch_ = 0;
    invisibility_present_pending_ = false;
    invisibility_presentations_.clear();
    for (auto& side : invisibility_side_presentations_) { side.clear(); }
    invisibility_presentation_cursor_ = 0;
    write_invisibility_world_ = false;
    read_world_background_ = false;
    unit_bitmaps_.clear();
    pending_effect_presentations_.clear();
    movement_effect_stamps_.clear();
    last_reaction_.clear();
    reaction_draws_ = 0;
    map_id = id;
    texture_placements_.clear();
    composing_static_ = true;
    cleared_object_cells_.clear();
    dynamic_objects_.clear();
    removed_dynamic_cells_.clear();
    size_t m = item(6, id - 1);
    width = word(prg_, m);
    height = word(prg_, m + 2);
    if (width < 10 || height < 15 || width > 256 || height > 256 || word(prg_, m + 4) != width * height * 2)
    {
        fatal("Invalid original staggered map geometry");
    }
    // sub_4293AF -> sub_474AE7/sub_474B1A clears the indexed surface to zero.
    pixels_.assign(size_t(width) * 64 * height * 32, 0);
    // 4293AF -> 42FD9F: opaque backdrop, terrain, then transparent foreground backdrop.
    if (unsigned background = word(prg_, m + 16))
    {
        blit(backdrop(background), int(int16_t(word(prg_, m + 18))), int(int16_t(word(prg_, m + 20))), false);
    }
    size_t cells = m + 28;
    size_t scenery = cells + size_t(width) * height * 10;
    size_t lookups = scenery + word(prg_, m + 12) * 10 + word(prg_, m + 14) * 6;
    std::array<std::vector<unsigned>, 3> mapping;
    for (unsigned a = 0; a < 3; ++a)
    {
        unsigned used = word(prg_, lookups);
        lookups += 2;
        unsigned count = word(prg_, group(3 + a));
        for (unsigned i = 0; i < count; ++i)
        {
            mapping[a].push_back(word(prg_, lookups + i * 2));
        }
        lookups += count * 2 + used * 8;
    }
    auto mapped = [&](unsigned layer, unsigned local) -> unsigned
    {
        if (!local || local > mapping[layer].size() || !mapping[layer][local - 1])
        {
            fatal("Missing original map tile mapping");
        }
        return mapping[layer][local - 1] - 1;
    };
    auto draw_scenery = [&](size_t p)
    {
        int sx = int(int16_t(word(prg_, p))), sy = int(int16_t(word(prg_, p + 2)));
        int sw = int(int16_t(word(prg_, p + 4))), sh = int(int16_t(word(prg_, p + 6)));
        int bx = sx + sw / 128, by = sy + sh / 16 - 2, elevation = 0;
        if (bx >= 0 && bx < int(width) && by >= 1 && by < int(height * 2) && ((by & 1) || (bx != 0 && bx != int(width) - 1)))
        {
            elevation = prg_.at(cells + (size_t(by) * width + bx) * 5);
        }
        unsigned scenery_id = word(prg_, p + 8);
        blit(raw(item(5, scenery_id - 1)), sx * 64 + (sy & 1) * 32, sy * 16 - std::max(0, elevation - 1) * 16, true, true);
        // 42EB5E crops a copy of each eligible object before compositing it again.
        int depth = scenery_attributes_.at(scenery_id - 1), rows = sh / 16 + 1, columns = sw / 64 + 2;
        int row = depth ? sy - 1 + rows - depth : sy - 1;
        for (int i = 0; i < (depth ? depth : rows) && row < int(height * 2); ++i, ++row)
        {
            if (row < 0)
            {
                continue;
            }
            int phase = depth ? depth - i : i;
            int start = sx - 1;
            if ((phase & 1) && (row & 1)) { start = sx; }

            for (int j = 0; j < columns; ++j)
            {
                int x = start + j;
                if (row < 1 || x < 0 || x >= int(width) || (row & 1 ? x >= int(width) - 1 : x == 0))
                {
                    continue;
                }
                size_t cell = cells + (size_t(row) * width + x) * 5;
                unsigned object = word(prg_, cell + 3);
                if (!object)
                {
                    continue;
                }
                unsigned type = object_attributes_.at(object - 1) >> 8;
                if (type == 12 || (type == 5 && !event_flags_[1]) || (type == 6 && !event_flags_[0]) || (type == 11 && !event_flags_[3]))
                {
                    continue;
                }
                if (!depth)
                {
                    bool sealed = false;
                    size_t special = scenery + word(prg_, m + 12) * 10;
                    for (unsigned k = 0; k < word(prg_, m + 14); ++k)
                    {
                        size_t q = special + k * 6;
                        if (int(int16_t(word(prg_, q))) == x && int(int16_t(word(prg_, q + 2))) == row)
                        {
                            sealed = (word(prg_, q + 4) & 0x4000) != 0;
                            break;
                        }
                    }
                    if (!sealed)
                    {
                        continue;
                    }
                }
                auto image = raw(item(4, object - 1));
                int level = prg_.at(cell);
                std::vector<uint8_t> masked = image.pixels;
                mask_unit_ground(masked, image.w, image.h, x, row, level);
                image.pixels = SharedBytes(std::move(masked));
                blit(image, x * 64 + (row & 1) * 32, row * 16 - (level - 1) * 16 - 64, true, true);
            }
        }
    };
    for (unsigned y = 0; y < height * 2; ++y)
    {
        for (unsigned i = 0; i < word(prg_, m + 12); ++i)
        {
            size_t p = scenery + i * 10;
            if (int(int16_t(word(prg_, p + 2))) * 16 + int(int16_t(word(prg_, p + 6))) == int(y * 16))
            {
                draw_scenery(p);
            }
        }
        for (unsigned x = 0; x < width - (y & 1); ++x)
        {
            size_t p = cells + (size_t(y) * width + x) * 5;
            unsigned elevation = prg_.at(p), ground = word(prg_, p + 1), object = word(prg_, p + 3);
            if (!elevation)
            {
                continue;
            }
            const auto& tile = raw(item(3, ground - 1));
            unsigned layers = ground <= 5 ? 1 : elevation;
            // 0x42E89C passes zero to 0x4753A4: diamond padding must preserve earlier rows.
            for (unsigned z = 0; z < layers; ++z)
            {
                blit(tile, int(x * 64 + (y & 1) * 32), int(y * 16) - int(z * 16), true);
            }
            // sub_42E89C returns immediately after drawing ground IDs 1..5.
            if (ground > 5 && object)
            {
                unsigned type = object_attributes_.at(object - 1) >> 8;
                if (type == 12 || (type == 5 && !event_flags_[1]) || (type == 6 && !event_flags_[0]) || (type == 11 && !event_flags_[3]))
                {
                    continue;
                }
                const auto& obj = raw(item(4, object - 1));
                blit(obj, int(x * 64 + (y & 1) * 32), int(y * 16) - int((layers - 1) * 16) - 64, true, true);
            }
        }
    }
    // 42EAAF draws scenery whose bottom extends past the final scanned row.
    for (unsigned i = 0; i < word(prg_, m + 12); ++i)
    {
        size_t p = scenery + i * 10;
        if (int(int16_t(word(prg_, p + 2))) * 16 + int(int16_t(word(prg_, p + 6))) > int(height * 32) - 16)
        {
            draw_scenery(p);
        }
    }
    if (unsigned foreground = word(prg_, m + 22))
    {
        blit(backdrop(foreground), int(int16_t(word(prg_, m + 24))), int(int16_t(word(prg_, m + 26))), true);
    }
    // 42F8B7 validates four frame IDs; 42FBC6 draws the original current object.
    std::vector<std::pair<int, int>> dynamic_cells;
    unsigned chain_locations = 0;
    if (!dynamic_object_chains_.empty())
    {
        for (unsigned y = 0; y < height * 2; ++y)
        {
            for (unsigned x = 0; x < width - (y & 1); ++x)
            {
                size_t cell = cells + (size_t(y) * width + x) * 5;
                unsigned object = word(prg_, cell + 3);
                unsigned type = object ? object_attributes_.at(object - 1) >> 8 : 0;
                if (type == 35)
                {
                    continue;
                }
                if (type == 12)
                {
                    dynamic_cells.emplace_back(x, y);
                    ++chain_locations;
                    continue;
                }
                size_t special = scenery + word(prg_, m + 12) * 10;
                for (unsigned i = 0; i < word(prg_, m + 14); ++i)
                {
                    size_t p = special + i * 6;
                    if (int(int16_t(word(prg_, p))) == int(x) && int(int16_t(word(prg_, p + 2))) == int(y))
                    {
                        unsigned chain = prg_.at(p + 4);
                        if (chain && chain <= 20)
                        {
                            ++chain_locations;
                        }
                        break;
                    }
                }
            }
        }
    }
    if (chain_locations > dynamic_object_chains_.size())
    {
        return Result<bool>::failure(make_error("#E015: original map chain locations exceed stage chain count"));
    }
    for (auto [x, y] : dynamic_cells)
    {
        size_t special = scenery + word(prg_, m + 12) * 10;
        unsigned chain = 0;
        for (unsigned i = 0; i < word(prg_, m + 14); ++i)
        {
            size_t p = special + i * 6;
            if (int(int16_t(word(prg_, p))) == x && int(int16_t(word(prg_, p + 2))) == y)
            {
                chain = prg_.at(p + 4);
                break;
            }
        }
        if (chain && chain <= dynamic_object_chains_.size())
        {
            const auto& frames = dynamic_object_chains_[chain - 1];
            if (std::any_of(frames.begin(), frames.end(), [](unsigned frame)
                    {
                        return !frame || frame > 302;
                    }))
            {
                cleared_object_cells_.push_back(cells + (size_t(y) * width + x) * 5);
            }
            else
            {
                int left = x * 64 + (y & 1) * 32, top = y * 16 - (int(prg_.at(cells + (size_t(y) * width + x) * 5)) - 1) * 16 - 64;
                Image background{ 64, 96, SharedBytes(std::vector<uint8_t>(64 * 96)), {} };
                for (int row = 0; row < 96; ++row)
                {
                    for (int col = 0; col < 64; ++col)
                    {
                        int dx = left + col, dy = top + row;
                        if (dx >= 0 && dy >= 0 && dx < int(width * 64) && dy < int(height * 32))
                        {
                            background.pixels.mutable_at(size_t(row) * 64 + col) = pixels_[size_t(dy) * width * 64 + dx];
                        }
                    }
                }
                dynamic_objects_.push_back({ x, y, left, top, std::move(background) });
            }
        }
    }
    composing_static_ = false;
    for (auto [x, y] : dynamic_cells)
    {
        size_t cell = cells + (size_t(y) * width + x) * 5;
        unsigned object = cell_object(cell);
        int elevation = prg_.at(cell);
        if (!object)
        {
            continue;
        }
        auto image = raw(item(4, object - 1));
        std::vector<uint8_t> masked = image.pixels;
        mask_unit_ground(masked, image.w, image.h, x, y, elevation);
        image.pixels = SharedBytes(std::move(masked));
        blit(image, x * 64 + (y & 1) * 32, y * 16 - (elevation - 1) * 16 - 64, true);
    }
    camera_x = 32;
    camera_y = 16;
    constrain_camera();
    return Result<bool>::success(true);
}

auto OriginalMap::try_screen_position(int x, int y) const -> Result<std::pair<int, int>>
{
    if (x < 0 || y < 0 || unsigned(y) >= height * 2u || unsigned(x) >= width - (unsigned(y) & 1))
    {
        return Result<std::pair<int, int>>::failure(make_error("Invalid original map cell"));
    }
    const size_t cell = item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5;
    const int h = int(prg_.at(cell));
    // sub_42C50A uses the staggered cell's left/top, not its center.
    return Result<std::pair<int, int>>::success({ x * 64 + (y & 1) * 32 - camera_x, y * 16 - camera_y - std::max(0, h - 1) * 16 });
}

auto OriginalMap::dimensions() const -> std::pair<unsigned, unsigned> { return { width, height }; }

auto OriginalMap::overview(int output_width, int output_height) -> std::vector<uint8_t>
{
    if (output_width <= 0 || output_height <= 0 || !width || !height) { fatal("Invalid map overview dimensions"); }
    const int saved_x = camera_x, saved_y = camera_y;
    std::vector<uint8_t> result(size_t(output_width) * output_height * 4);
    std::map<std::pair<int, int>, std::vector<uint8_t>> tiles;
    for (int y = 0; y < output_height; ++y)
    {
        for (int x = 0; x < output_width; ++x)
        {
            const int sx = x * int(width * 64) / output_width, sy = y * int(height * 32) / output_height;
            camera_x = (sx / 640) * 640;
            camera_y = (sy / 480) * 480;
            const auto key = std::pair{ camera_x, camera_y };
            if (!tiles.contains(key)) { tiles.emplace(key, draw()); }
            const auto& tile = tiles.at(key);
            const size_t source = (size_t(sy - camera_y) * 640 + sx - camera_x) * 4;
            std::copy_n(tile.begin() + source, 4, result.begin() + (size_t(y) * output_width + x) * 4);
        }
    }
    camera_x = saved_x;
    camera_y = saved_y;
    return result;
}

auto OriginalMap::screen_position(int x, int y) const -> std::pair<int, int> { return try_screen_position(x, y).value(); }

auto OriginalMap::try_completion_location(unsigned completion) const -> Result<std::pair<int, int>>
{
    unsigned category = completion == 1 ? 6u : 11u;
    size_t base = item(6, map_id - 1) + 28;
    std::pair<int, int> selected{};
    // 原42D3B4按行列扫描全部匹配对象，输出最后一项，不按角色位置替代。
    for (unsigned y = 0; y < height * 2; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            unsigned object = word(prg_, base + (size_t(y) * width + x) * 5 + 3);
            if (object && object <= object_attributes_.size() && (object_attributes_[object - 1] >> 8) == category)
            {
                selected = { int(x), int(y) };
            }
            if (selected.first || selected.second) { return Result<std::pair<int, int>>::success(selected); }
        }
    }
    return Result<std::pair<int, int>>::failure(make_error("E070 original completion map object missing"));
}

auto OriginalMap::draw_completion_location(const OriginalMenu& font, std::vector<uint8_t>& image, int x, int y) const -> Result<bool>
{
    size_t base = item(6, map_id - 1) + 28;
    unsigned object = word(prg_, base + (size_t(y) * width + unsigned(x)) * 5 + 3);
    if (!object || object > object_attributes_.size()) { return Result<bool>::failure(make_error("Original completion object name unavailable")); }
    unsigned local = object_attributes_[object - 1] & 255;
    if (!local || local > terrain_object_names_) { return Result<bool>::failure(make_error("Original completion object name index")); }
    size_t start = size_t(terrain_ground_names_ + local - 1) * 11;
    std::vector<uint8_t> name;
    for (size_t i = start; i < start + 11 && terrain_names_.at(i); ++i) { name.push_back(terrain_names_[i]); }
    auto p = try_screen_position(x, y);
    if (!p) { return Result<bool>::failure(p.error()); }
    int sx = std::clamp(p.value().first + 32 - int(name.size()) * 6, 5, 510);
    int sy = std::clamp(p.value().second, 5, 450);
    return font.try_draw_original_large(image, name, sx, sy, 99, 167);
}

auto OriginalMap::dialogue_visible(int x, int y, unsigned mode) const -> bool
{
    int half = camera_x / 32, row = camera_y / 16;
    if (y <= 0 || y >= int(height * 2) - 1 || x < 0 || x >= int(width) - (y & 1))
    {
        return false;
    }
    if (!(y & 1) && (x == 0 || x == int(width) - 1))
    {
        return false;
    }
    if (half & 1)
    {
        if (x < (half >> 1) || ((y & 1) ? x >= (half >> 1) + 10 : x > (half >> 1) + 10))
        {
            return false;
        }
    }
    else if (x < (half >> 1) - ((y & 1) ? 1 : 0) || x >= (half >> 1) + 10)
    {
        return false;
    }
    const size_t cell = item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5;
    y -= std::max(0, int(prg_.at(cell)) - 1);
    return y >= row + (mode == 2 ? 2 : -1) && y < row + 30;
}

auto OriginalMap::dialogue_camera(int x, int y) const -> std::pair<int, int>
{
    const size_t cell = item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5;
    int half = 2 * x - ((y & 1) ? 8 : 9), row = y - (int(prg_.at(cell)) - 1 + 16);
    if ((half + 21) / 2 >= int(width))
    {
        half = int(width) * 2 - 21;
    }
    if (row + 30 >= int(height) * 2)
    {
        row = int(height) * 2 - 30;
    }
    return constrained_camera(std::max(1, half) * 32, std::max(1, row) * 16);
}

auto OriginalMap::movement_camera(int x, int y) -> void
{
    // 452167 calls 42B37A(mode1) immediately for every route cell.
    const size_t cell = item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5;
    int half = 2 * x - ((y & 1) ? 8 : 9), row = y - (int(prg_.at(cell)) - 1 + 16);
    if ((half + 21) / 2 >= int(width))
    {
        half = int(width) * 2 - 21;
    }
    if (row + 30 >= int(height) * 2)
    {
        row = int(height) * 2 - 30;
    }
    camera_x = std::max(1, half) * 32;
    camera_y = (row < 1 ? 2 : row) * 16;
    constrain_camera();
}

auto OriginalMap::try_idle_frame(unsigned appearance, uint64_t elapsed_ms, unsigned direction, unsigned action) const -> Result<unsigned>
{
    size_t index = group(1);
    unsigned count = word(prg_, index);
    if (!appearance || appearance > count)
    {
        return Result<unsigned>::failure(make_error("Invalid original unit appearance"));
    }
    size_t animation = index + 2 + count * 8 + (appearance - 1) * 200;
    const size_t end = animation + 200;
    // sub_444BCF builds offsets for consecutive zero-terminated action/direction sequences.
    unsigned requested = action * 4 + (direction & 3);
    if (requested >= 30)
    {
        return Result<unsigned>::failure(make_error("Invalid original unit action sequence"));
    }
    for (unsigned sequence = 0; sequence < requested; ++sequence)
    {
        while (animation < end && prg_.at(animation))
        {
            ++animation;
        }
        if (++animation >= end)
        {
            return Result<unsigned>::failure(make_error("Missing original direction sequence"));
        }
    }
    size_t length = 0;
    // sub_444BCF/sub_447DB6: zero-terminated sequences, not 200 animation frames.
    while (animation + length < end && prg_.at(animation + length))
    {
        ++length;
    }
    if (!length || animation + length == end)
    {
        return Result<unsigned>::failure(make_error("Invalid original unit idle sequence"));
    }
    // sub_43A51B checks 0x13 centiseconds via sub_473192/sub_47308B.
    return Result<unsigned>::success(prg_.at(animation + size_t((elapsed_ms / 190) % length)));
}

auto OriginalMap::idle_frame(unsigned appearance, uint64_t elapsed_ms, unsigned direction, unsigned action) const -> unsigned { return try_idle_frame(appearance, elapsed_ms, direction, action).value(); }

auto OriginalMap::try_draw_status_figure(std::vector<uint8_t>& image, const OriginalEvents::Unit& unit, int center, int bottom) const -> Result<bool>
{
    SharedBytes source = prg_;
    size_t frames = group(2);
    unsigned selected = 0;
    if (unit.status_flags & 0x10)
    {
        // 458621/44522E：召唤兵使用 CEL 第4/5组，+2Ch 是军团序列游标，不是朝向字节 +2Bh。
        if (!unit.action_status_runtime) { return Result<bool>::failure(make_error("Special status figure requires runtime +2C pose cursor")); }
        auto cel = read_shared_resource(root_ / "SUPERSP.CEL");
        if (!cel) { return Result<bool>::failure(cel.error()); }
        source = std::move(cel).value();
        const unsigned raw_index = dword(pal_, 808 + 3 * 8), raw_frames = dword(pal_, 808 + 4 * 8);
        if (raw_index < 59 || raw_frames < 60) { return Result<bool>::failure(make_error("Invalid original summon archive address")); }
        const size_t index = raw_index - 59;
        frames = raw_frames - 60;
        ResourceReader summon(source);
        const unsigned count = summon.number(index);
        if (!summon) { return Result<bool>::failure(summon.error()); }
        if (unit.appearance < 0x71 || unit.appearance - 0x71 >= count) { return Result<bool>::failure(make_error("Invalid original summon class")); }
        const unsigned entry = unit.appearance - 0x71;
        const unsigned first = summon.number(index + 2 + size_t(entry) * 8 + 2);
        const unsigned cursor = unit.action_status_runtime->at(0x2C);
        if (cursor >= 200) { return Result<bool>::failure(make_error("Invalid original summon pose cursor")); }
        const unsigned pose = summon.number(index + 2 + size_t(count) * 8 + size_t(entry) * 200 + cursor, 1);
        if (!summon) { return Result<bool>::failure(summon.error()); }
        if (!pose) { return Result<bool>::failure(make_error("Empty original summon pose")); }
        selected = first + pose - 1;
    }
    else
    {
        auto frame = try_idle_frame(unit.appearance, 0, 0, 0);
        if (!frame) { return Result<bool>::failure(frame.error()); }
        // 普通状态立绘保留 PRG 基帧，不跟随地图朝向或动作。
        selected = word(prg_, group(1) + 2 + size_t(unit.appearance - 1) * 8 + 2);
    }
    ResourceReader reader(source);
    const unsigned count = reader.number(frames);
    if (!reader) { return Result<bool>::failure(reader.error()); }
    if (selected >= count) { return Result<bool>::failure(make_error("Invalid original status figure frame")); }
    const size_t table = frames + 2 + size_t(selected) * 7;
    const size_t offset = frames + reader.number(table, 4);
    const size_t length = reader.number(table + 4);
    const unsigned width = reader.number(offset) + 1, height = reader.number(offset + 2) + 1;
    if (!reader.extent(offset, length)) { return Result<bool>::failure(reader.error()); }
    auto decoded = OriginalSkill32Visual::try_decode({ source.begin() + offset, source.begin() + offset + length }, width, height);
    if (!decoded) { return Result<bool>::failure(decoded.error()); }
    const int left = center - int(width / 2), top = bottom - int(height);
    if (image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("Invalid status figure framebuffer")); }
    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            const unsigned color = decoded.value().pixels[size_t(y) * width + x];
            const int dx = left + int(x), dy = top + int(y);
            if (!color || dx < 0 || dx >= 640 || dy < 0 || dy >= 480) { continue; }
            const size_t dest = (size_t(dy) * 640 + dx) * 4;
            for (unsigned k = 0; k < 3; ++k) { image[dest + k] = uint8_t(unsigned(pal_[color * 3 + k]) * 255 / 63); }
            image[dest + 3] = 255;
        }
    }
    return Result<bool>::success(true);
}

auto OriginalMap::unit_height(unsigned appearance, unsigned direction, unsigned action) const -> unsigned
{
    size_t index = group(1), frames = group(2);
    unsigned frame = idle_frame(appearance, 0, direction, action);
    unsigned first = word(prg_, index + 2 + (appearance - 1) * 8 + 2);
    size_t p = frames + dword(prg_, frames + 2 + (first + frame - 1) * 7);
    return word(prg_, p + 2) + 1;
}

auto OriginalMap::expected_reaction_bitmap(unsigned id, bool left) const -> std::vector<uint8_t>
{
    const auto& bitmap = unit_bitmaps_.at(id);
    return compose_reaction(bitmap.w, bitmap.h, bitmap.ad, bitmap.b1, left);
}

auto OriginalMap::unit_background_bitmap(unsigned id) const -> const std::vector<uint8_t>& { return unit_bitmaps_.at(id).b1; }

auto OriginalMap::unit_source_bitmap(unsigned id) const -> const std::vector<uint8_t>& { return unit_bitmaps_.at(id).ad; }

auto OriginalMap::unit_bitmap_bounds(unsigned id) const -> std::array<int, 4>
{
    const auto& b = unit_bitmaps_.at(id);
    return { b.left, b.top, int(b.w), int(b.h) };
}

auto OriginalMap::unit_bitmap_available(unsigned id) const -> bool { return unit_bitmaps_.contains(id); }

auto OriginalMap::begin_teleport_arrival(const std::vector<OriginalEvents::Unit>& roster, unsigned target, unsigned turn, bool departure, uint64_t elapsed, unsigned color) -> bool
{
    if (arrival_target_ || target >= roster.size())
    {
        return false;
    }
    auto image = draw();
    auto prepared = try_draw_units(image, roster, turn, elapsed);
    if (!prepared) { return false; }
    if (!unit_bitmaps_.contains(target))
    {
        return false;
    }
    arrival_target_ = target;
    arrival_departure_ = departure;
    arrival_color_ = departure ? 254 : 194;
    if (color) { arrival_color_ = color; }
    arrival_frame_ = arrival_elapsed_ = 0;
    arrival_presented_ = false;
    arrival_pose_time_ = idle_pose_time_.value_or(elapsed);
    return true;
}

auto OriginalMap::teleport_arrival_active() const -> bool { return arrival_target_.has_value(); }

auto OriginalMap::begin_summon_growth(const std::vector<OriginalEvents::Unit>& roster, unsigned target, unsigned turn) -> Result<bool>
{
    if (summon_target_ || target >= roster.size()) { return Result<bool>::failure(make_error("Invalid465E04 growth target")); }
    auto candidate = *this;
    auto image = candidate.draw();
    auto prepared = candidate.try_draw_units(image, roster, turn);
    if (!prepared) { return prepared; }
    const auto found = candidate.unit_bitmaps_.find(target);
    if (found == candidate.unit_bitmaps_.end()) { return Result<bool>::failure(make_error("465E04 summon AD unavailable")); }
    candidate.summon_full_rows_ = found->second.h;
    candidate.summon_rows_ = found->second.h % 3 + 1;
    candidate.summon_target_ = target;
    candidate.summon_presented_ = false;
    *this = std::move(candidate);
    return Result<bool>::success(true);
}

auto OriginalMap::summon_growth_active() const -> bool { return summon_target_.has_value(); }

auto OriginalMap::summon_growth_target() const -> std::optional<unsigned> { return summon_target_; }

auto OriginalMap::summon_growth_rows() const -> unsigned { return summon_rows_; }

auto OriginalMap::summon_growth_presented() -> void { summon_presented_ = true; }

auto OriginalMap::tick_summon_growth() -> void
{
    if (!summon_target_ || !summon_presented_) { return; }
    summon_presented_ = false;
    if (summon_rows_ == summon_full_rows_)
    {
        summon_target_.reset();
        return;
    }
    summon_rows_ = std::min(summon_full_rows_, summon_rows_ + 3);
}

auto OriginalMap::cancel_summon_growth() -> void { summon_target_.reset(); }

auto OriginalMap::teleport_arrival_frame() const -> unsigned { return arrival_frame_; }

auto OriginalMap::teleport_arrival_presented() -> void { arrival_presented_ = true; }

auto OriginalMap::tick_teleport_arrival(unsigned ms) -> void
{
    if (!arrival_target_ || !arrival_presented_)
    {
        return;
    }
    arrival_elapsed_ += std::min(ms, 40u);
    if (arrival_elapsed_ < 40)
    {
        return;
    }
    arrival_elapsed_ = 0;
    arrival_presented_ = false;
    if (++arrival_frame_ == 10)
    {
        arrival_target_.reset();
    }
}

auto OriginalMap::cancel_teleport_arrival() -> void { arrival_target_.reset(); }

auto OriginalMap::begin_disappearance(const std::vector<OriginalEvents::Unit>& roster, unsigned target, unsigned turn, uint64_t elapsed, bool death, std::string* call_error) -> bool
{
    if (call_error) { call_error->clear(); }
    if (disappearance_.active())
    {
        if (call_error) { *call_error = "Disappearance already active target=" + std::to_string(disappearance_.target); }
        return false;
    }
    auto role = target < roster.size() ? roster.begin() + target : roster.end();
    if (role == roster.end() || !role->present(turn) || !(role->status_flags & 0x80) || (role->status_flags & 0x20)
        || (role->appearance && role->appearance > word(prg_, group(1)))
        || (!role->appearance && !unit_bitmaps_.contains(target)))
    {
        if (call_error)
        {
            *call_error = "Disappearance target not drawable at turn=" + std::to_string(turn);
            if (role != roster.end())
            {
                *call_error += " hp=" + std::to_string(role->hp) + " flags=" + std::to_string(role->status_flags)
                    + " first=" + std::to_string(role->first_turn) + " last=" + std::to_string(role->last_turn)
                    + " appearance=" + std::to_string(role->appearance) + " limit=" + std::to_string(word(prg_, group(1)));
            }
        }
        return false;
    }
    // Initialize full indexed AD/B1; do not capture an RGBA framebuffer.
    auto scratch = draw();
    auto prepared = try_draw_units(scratch, roster, turn, elapsed);
    if (!prepared)
    {
        if (call_error) { *call_error = prepared.error().message; }
        return false;
    }
    auto found_target = unit_bitmaps_.find(target);
    if (found_target == unit_bitmaps_.end())
    {
        if (call_error) { *call_error = "Disappearance target bitmap not prepared"; }
        return false;
    }
    auto& bitmap = found_target->second;
    auto background = bitmap.b1;
    for (const auto& other : roster)
    {
        if (unsigned(&other - roster.data()) == target || !other.present(turn) || (other.status_flags & 0x20) || !(other.status_flags & 0x80) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5)
        {
            continue;
        }
        auto found = unit_bitmaps_.find(unsigned(&other - roster.data()));
        if (found == unit_bitmaps_.end())
        {
            continue;
        }
        const auto& source = found->second;
        if (std::abs(source.x - bitmap.x) > 2 || source.y >= bitmap.y || source.y < bitmap.y - 24)
        {
            continue;
        }
        overlay_bitmap(bitmap, source, false);
    }
    auto masked = bitmap.ad;
    int elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(role->y) * width + role->x) * 5);
    const int displacement = int(int8_t(prg_.at(bitmap.frame + 6)));
    // 442452 -> 448984 adds the original indexed foot shadow before masking.
    if (!(role->status_flags & 8) && bitmap.h >= 32)
    {
        if (reaction_foot_mask_.empty())
        {
            (void)compose_reaction(bitmap.w, bitmap.h, bitmap.ad, bitmap.b1, true);
        }
        int base = int((bitmap.h - 32) * bitmap.w + bitmap.w / 2) - 32 - displacement;
        int back = int((bitmap.h - 32) * bitmap.w + bitmap.w / 2) - 32;
        for (int y = 0; y < 32; ++y)
        {
            for (int x = 0; x < 64; ++x)
            {
                int p = base + y * int(bitmap.w) + x, q = back + y * int(bitmap.w) + x;
                if (p >= 0 && q >= 0 && p < int(masked.size()) && q < int(bitmap.b1.size()) && reaction_foot_mask_[size_t(y) * 64 + x] && !masked[size_t(p)])
                {
                    masked[size_t(p)] = reaction_shadow_[bitmap.b1[size_t(q)]];
                }
            }
        }
    }
    mask_unit_ground(masked, bitmap.w, bitmap.h, role->x, role->y, elevation, checker_mask_, displacement);
    // 44689B/4468DF mode1 -> 423D76: nearer sprites erase scratch,
    // rather than blending those occluders into the disappearing role.
    for (const auto& other : roster)
    {
        if (!other.present(turn) || (other.status_flags & 0x20) || !(other.status_flags & 0x80) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5)
        {
            continue;
        }
        auto found = unit_bitmaps_.find(unsigned(&other - roster.data()));
        if (found == unit_bitmaps_.end())
        {
            continue;
        }
        const auto& source = found->second;
        int minimum = bitmap.x - 2 - (displacement < 0), maximum = bitmap.x + 2 + (displacement > 0);
        if (source.x < minimum || source.x > maximum || source.y <= bitmap.y || source.y > bitmap.y + 24)
        {
            continue;
        }
        for (unsigned y = 0; y < source.h; ++y)
        {
            for (unsigned x = 0; x < source.w; ++x)
            {
                int dx = source.left - bitmap.left - displacement + int(x), dy = source.top - bitmap.top + int(y);
                if (dx >= 0 && dy >= 0 && dx < int(bitmap.w) && dy < int(bitmap.h) && source.ad[size_t(y) * source.w + x])
                {
                    masked[size_t(dy) * bitmap.w + unsigned(dx)] = 0;
                }
            }
        }
    }
    disappearance_.begin(target, masked, bitmap.b1);
    bitmap.b1 = std::move(background);
    if (death)
    {
        disappearance_.death_mode();
    }
    else
    {
        disappearance_.frame_count = 5;
        disappearance_.frame_interval = 20;
        disappearance_.first_weight = 40;
    }
    const auto& palette = *palette_;
    disappearance_frames_.clear();
    auto copy = disappearance_;
    if (death)
    {
        for (auto& color : copy.source)
        {
            if (color)
            {
                color = uint8_t(palette.blend_index(color, 0x3C, 80));
            }
        }
    }
    for (unsigned frame = 0; frame < copy.frame_count; ++frame)
    {
        copy.frame = frame;
        disappearance_frames_.push_back(copy.pixels([&](unsigned a, unsigned b, int weight)
            {
                return palette.blend_index(a, b, weight);
            }));
    }
    return true;
}

auto OriginalMap::enable_disappearance_presentation() -> void { disappearance_.enable_presentation_timing(); }

auto OriginalMap::disappearance_presented() -> void { disappearance_.presented(); }

auto OriginalMap::tick_disappearance(unsigned ms) -> void { disappearance_.tick(ms); }

auto OriginalMap::disappearance_active() const -> bool { return disappearance_.active(); }

auto OriginalMap::disappearance_finished() const -> bool { return disappearance_.finished(); }

auto OriginalMap::disappearance_frame() const -> unsigned { return disappearance_.frame; }

auto OriginalMap::disappearance_pixels() const -> const std::vector<uint8_t>& { return disappearance_frames_.at(std::min(disappearance_.frame, disappearance_.frame_count - 1)); }

auto OriginalMap::last_reaction_target() const -> unsigned { return last_reaction_target_; }

auto OriginalMap::last_reaction_left() const -> bool { return last_reaction_left_; }

auto OriginalMap::reaction_draws() const -> uint64_t { return reaction_draws_; }

auto OriginalMap::last_reaction_bitmap() const -> const std::vector<uint8_t>& { return last_reaction_; }

auto OriginalMap::draw_units_validated(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, unsigned turn, uint64_t elapsed_ms, std::optional<unsigned> silhouette, std::optional<unsigned> reaction, unsigned background_mode, std::optional<unsigned> only_instance, std::vector<uint8_t>* indexed) const -> Result<bool>
{
    const auto& palette = *palette_;
    size_t index = group(1), frames = group(2);
    unsigned count = word(prg_, index);
    struct Presented
    {
        unsigned instance;
        OriginalEvents::Unit unit;
        unsigned w, h;
        int left, top;
        std::vector<uint8_t> pixels;
    };
    std::vector<Presented> presented;
    std::vector<unsigned> changed;
    std::optional<bool> external_units_available;
    for (const auto& unit : units)
    {
        unsigned instance = unsigned(&unit - units.data());
        if (only_instance && instance != *only_instance)
        {
            continue;
        }
        if (!unit.present(turn) || (unit.status_flags & 0x20) || !(unit.status_flags & 0x80) || !unit.appearance || unit.appearance > count)
        {
            continue;
        }
        size_t metadata = index + 2 + (unit.appearance - 1) * 8;
        unsigned first = word(prg_, metadata + 2);
        uint64_t pose_time = unit.movement_pose ? uint64_t(unit.movement_frame) * 190 : idle_pose_time_.value_or(elapsed_ms);
        if (summon_target_) { pose_time = 0; }
        if (arrival_target_) { pose_time = arrival_pose_time_; }
        auto frame_result = try_idle_frame(unit.appearance, pose_time, unit.direction, unit.animation_action);
        if (!frame_result)
        {
            return Result<bool>::failure(frame_result.error());
        }
        unsigned frame = frame_result.value();
        size_t table = frames + 2 + (first + frame - 1) * 7;
        if (disappearance_.active() && instance == disappearance_.target)
        {
            table = unit_bitmaps_.at(instance).frame;
        }
        size_t p = frames + dword(prg_, table);
        unsigned length = word(prg_, table + 4);
        if (p > prg_.size() || prg_.size() - p < 4)
        {
            return Result<bool>::failure(make_error("Truncated original unit sprite header"));
        }
        unsigned w = word(prg_, p) + 1, h = word(prg_, p + 2) + 1;
        if (w > 512 || h > 512 || length < 4 || p > prg_.size() || length > prg_.size() - p)
        {
            return Result<bool>::failure(make_error("Invalid original unit sprite"));
        }
        std::vector<uint8_t> decoded;
        decoded.reserve(size_t(w) * h);
        size_t end = p + length;
        p += 4;
        // 424F70 consumes the entire entry; signed -128 is an empty literal.
        while (p < end)
        {
            unsigned code = prg_.at(p++), n = code ? code & 127 : 1;
            if (n > size_t(w) * h - decoded.size())
            {
                return Result<bool>::failure(make_error("Invalid original unit run"));
            }
            if (!code)
            {
                decoded.push_back(0);
                continue;
            }
            unsigned source = (code & 128) ? n : 1;
            if (source > end - p)
            {
                return Result<bool>::failure(make_error("Truncated original unit run"));
            }
            if (code & 128)
            {
                while (n--)
                {
                    decoded.push_back(prg_.at(p++));
                }
            }
            else
            {
                unsigned c = prg_.at(p++);
                decoded.insert(decoded.end(), n, uint8_t(c));
            }
        }
        if (decoded.size() != size_t(w) * h)
        {
            return Result<bool>::failure(make_error("Truncated original unit sprite"));
        }
        auto& bitmap = unit_bitmaps_[instance];
        if (!silhouette)
        {
            if (bitmap.w != w || bitmap.h != h || bitmap.frame != table || bitmap.x != unit.x || bitmap.y != unit.y || bitmap.state != unit.action_state || bitmap.status != unit.status_flags || background_mode != 0)
            {
                changed.push_back(instance);
            }
            bool resize = bitmap.w != w || bitmap.h != h;
            bitmap.w = w;
            bitmap.h = h;
            bitmap.frame = table;
            bitmap.x = unit.x;
            bitmap.y = unit.y;
            bitmap.state = unit.action_state;
            bitmap.status = unit.status_flags;
            bitmap.ad = decoded;
            bitmap.left = unit.x * 64 + (unit.y & 1) * 32 + 32 - int(w / 2);
            size_t cell = item(6, map_id - 1) + 28 + (size_t(unit.y) * width + unit.x) * 5;
            bitmap.top = unit.y * 16 + 32 - int(h) - (int(prg_.at(cell)) - 1) * 16;
            if (resize)
            {
                bitmap.b1.assign(size_t(w) * h, 0);
                for (unsigned y = 0; y < h; ++y)
                {
                    for (unsigned x = 0; x < w; ++x)
                    {
                        const int sx = bitmap.left + int(x), sy = bitmap.top + int(y);
                        if (sx >= 0 && sy >= 0 && sx < int(width * 64) && sy < int(height * 32))
                        {
                            const auto& background = read_world_background_ ? invisibility_world_ : pixels_;
                            bitmap.b1[size_t(y) * w + x] = background.at(size_t(sy) * width * 64 + sx);
                        }
                    }
                }
            }
        }
        if (silhouette)
        {
            if (*silhouette > 255)
            {
                return Result<bool>::failure(make_error("Invalid silhouette palette"));
            }
            for (auto& color : decoded)
            {
                if (color && color != 32)
                {
                    color = uint8_t(*silhouette);
                }
            }
        }
        if (arrival_target_ && instance == *arrival_target_ && !silhouette)
        {
            // 454ED5 tints AD before4462B6 composes feet and terrain masks.
            for (auto& ink : decoded)
            {
                if (ink)
                {
                    ink = palette.blend_index(ink, arrival_color_, arrival_departure_ ? 10 + int(arrival_frame_) * 10 : 100 - int(arrival_frame_) * 10);
                }
            }
        }
        // 4409A4 stores table+6 at runtime+ACh; 4462B6 sign-extends it.
        int displacement = int(int8_t(prg_.at(table + 6)));
        int left = unit.x * 64 + (unit.y & 1) * 32 + 32 - int(w / 2) + displacement - camera_x;
        size_t map = item(6, map_id - 1), cell = map + 28 + (size_t(unit.y) * width + unit.x) * 5;
        int elevation = prg_.at(cell);
        if (summon_target_ && instance == *summon_target_ && !silhouette)
        {
            // 465E04 shortens AD before42AFBA applies terrain masks at its new bottom anchor.
            h = std::min(h, summon_rows_);
            decoded.resize(size_t(w) * h);
        }
        if (!silhouette && (!summon_target_ || instance != *summon_target_ || summon_rows_ == summon_full_rows_))
        {
            if (unit.action_state == 2)
            {
                for (auto& ink : decoded)
                {
                    if (ink) { ink = uint8_t(palette.map(7, ink)); }
                }
            }
        }
        // 4483CB must finish B1 before442452 reads it for foot composition.
        int top = unit.y * 16 + 32 - int(h) - camera_y - std::max(0, elevation - 1) * 16;
        presented.push_back({ instance, unit, w, h, left, top, std::move(decoded) });
    }
    if (!silhouette)
    {
        for (auto id : changed)
        {
            auto& target = unit_bitmaps_.at(id);
            for (unsigned y = 0; y < target.h; ++y)
            {
                for (unsigned x = 0; x < target.w; ++x)
                {
                    const int sx = target.left + int(x), sy = target.top + int(y);
                    const auto& background = read_world_background_ ? invisibility_world_ : pixels_;
                    target.b1[size_t(y) * target.w + x] = sx >= 0 && sy >= 0 && sx < int(width * 64) && sy < int(height * 32) ? background.at(size_t(sy) * width * 64 + unsigned(sx)) : 0;
                }
            }
            if (background_mode == 1) { continue; }
            for (const auto& other : units)
            {
                if (!other.present(turn) || (other.status_flags & 0x20) || !(other.status_flags & 0x80) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5) { continue; }
                auto found = unit_bitmaps_.find(unsigned(&other - units.data()));
                if (found == unit_bitmaps_.end()) { continue; }
                const auto& source = found->second;
                if ((source.x == target.x && source.y == target.y) || std::abs(source.x - target.x) > 2 || std::abs(source.y - target.y) > 24) { continue; }
                overlay_bitmap(target, source, true);
            }
            if (background_mode != 2) { continue; }
            for (const auto& other : units)
            {
                if (!other.present(turn) || (other.status_flags & 0x20) || !(other.status_flags & 0x80) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5) { continue; }
                auto found = unit_bitmaps_.find(unsigned(&other - units.data()));
                if (found == unit_bitmaps_.end()) { continue; }
                const auto& source = found->second;
                if ((source.x == target.x && source.y == target.y) || std::abs(source.x - target.x) > 2 || source.y >= target.y || source.y < target.y - 24) { continue; }
                overlay_bitmap(target, source, false);
            }
        }
    }
    for (auto& pose : presented)
    {
        const auto& unit = pose.unit;
        const auto instance = pose.instance;
        auto& decoded = pose.pixels;
        const unsigned w = pose.w, h = pose.h;
        const size_t cell = item(6, map_id - 1) + 28 + (size_t(unit.y) * width + unit.x) * 5;
        const int displacement = pose.left + camera_x - (unit.x * 64 + (unit.y & 1) * 32 + 32 - int(w / 2));
        if (!silhouette && (unit.status_flags & 0x10)
            && (!summon_target_ || instance != *summon_target_ || summon_rows_ == summon_full_rows_))
        {
            if (h >= 32 && (!(unit.status_flags & 8) || (summon_target_ && instance == *summon_target_ && summon_rows_ == summon_full_rows_)))
            {
                const unsigned ground = word(prg_, cell + 1);
                const unsigned category = ground ? word(prg_, group(7) + 6 + (ground - 1) * 2) >> 8 : 0;
                const bool special = ground > 4 && (category == 1 || category == 2);
                auto& foot = invisibility_feet_[special ? 1 : 0];
                if (foot.empty())
                {
                    auto resources = OriginalDiscResources(root_).try_dat_entry(disc_, 5);
                    if (!resources) { return Result<bool>::failure(resources.error()); }
                    const auto& bytes = resources.value();
                    ResourceReader reader(bytes);
                    const size_t entry = 2 + size_t(special ? 21 : 20) * 8;
                    const size_t offset = reader.number(entry, 4), length = reader.number(entry + 4, 4);
                    if (length != 2052 || !reader.extent(offset, length) || reader.number(offset) != 63 || reader.number(offset + 2) != 31)
                    {
                        return Result<bool>::failure(make_error("Invalid465F8F final foot bitmap"));
                    }
                    foot.assign(bytes.begin() + offset + 4, bytes.begin() + offset + 2052);
                }
                auto shadow = NativeInvisibility::shadow(decoded, unit_bitmaps_.at(instance).b1, w, h, displacement,
                    foot, palette, special, unit.movement_type == 9);
                if (!shadow) { return shadow; }
            }
        }
        mask_unit_ground(decoded, w, h, unit.x, unit.y, int(prg_.at(cell)), checker_mask_, displacement);
    }
    std::stable_sort(presented.begin(), presented.end(), [](const auto& a, const auto& b)
        {
            return a.unit.y < b.unit.y;
        });
    std::vector<uint8_t> movement_indices;
    if (std::any_of(presented.begin(), presented.end(), [](const auto& pose)
            {
                return (pose.unit.action_state == 4 || pose.unit.movement_pose) && pose.unit.movement_visual_class >= 3 && pose.unit.movement_visual_class <= 5;
            }))
    {
        movement_indices.resize(640 * 480);
        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                movement_indices[size_t(y) * 640 + x] = indexed ? indexed->at(size_t(y) * 640 + x) : viewport_pixel(pixels_, x, y);
            }
        }
    }
    auto staged_effect_clock = movement_effect_clock_;
    auto staged_effect_stamps = movement_effect_stamps_;
    for (auto& pose : presented)
    {
        if (!silhouette && ((summon_target_ && pose.instance == *summon_target_) || (arrival_target_ && pose.instance == *arrival_target_)))
        {
            const auto& bitmap = unit_bitmaps_.at(pose.instance);
            const int displacement = int(int8_t(prg_.at(bitmap.frame + 6)));
            for (const auto& other : units)
            {
                if (!other.present(turn) || !(other.status_flags & 0x80) || (other.status_flags & 0x20) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5) { continue; }
                const auto found = unit_bitmaps_.find(unsigned(&other - units.data()));
                if (found == unit_bitmaps_.end()) { continue; }
                const auto& occluder = found->second;
                if (occluder.x < bitmap.x - 2 - (displacement < 0) || occluder.x > bitmap.x + 2 + (displacement > 0) || occluder.y <= bitmap.y || occluder.y > bitmap.y + 24) { continue; }
                auto erased = s2ne::try_erase(pose.pixels, pose.w, pose.h, occluder.ad, occluder.w, occluder.h,
                    occluder.left - camera_x - pose.left, occluder.top - camera_y - pose.top);
                if (!erased) { return erased; }
            }
        }
        const bool hidden = !silhouette && !(pose.unit.status39.value_or(0) & 2) && !(pose.unit.status3d & 0x20)
            && NativeInvisibility::hidden(pose.unit, units, turn, army_modes_[pose.unit.hostile() ? 0 : 1], [&](int x, int y)
                {
                    if (x < 0 || y < 0 || x >= int(width) || y >= int(height * 2)) { return 0; }
                    return int(prg_.at(item(6, map_id - 1) + 28 + (size_t(y) * width + x) * 5));
                });
        if (hidden)
        {
            const auto& bitmap = unit_bitmaps_.at(pose.instance);
            auto scratch = bitmap.ad;
            if (arrival_target_ && pose.instance == *arrival_target_)
            {
                for (auto& ink : scratch)
                {
                    if (ink) { ink = palette.blend_index(ink, arrival_color_, arrival_departure_ ? 10 + int(arrival_frame_) * 10 : 100 - int(arrival_frame_) * 10); }
                }
            }
            const size_t cell = item(6, map_id - 1) + 28 + (size_t(pose.unit.y) * width + pose.unit.x) * 5;
            const unsigned ground = word(prg_, cell + 1);
            const unsigned category = ground ? word(prg_, group(7) + 6 + (ground - 1) * 2) >> 8 : 0;
            const bool special = ground > 4 && (category == 1 || category == 2);
            auto& foot = invisibility_feet_[special ? 1 : 0];
            if (foot.empty())
            {
                auto resources = OriginalDiscResources(root_).try_dat_entry(disc_, 5);
                if (!resources) { return Result<bool>::failure(resources.error()); }
                const auto& data = resources.value();
                ResourceReader reader(data);
                // 426598 stores one-based slots at 4F7060 + slot*4: B4=21, B8=22.
                const size_t entry = 2 + size_t(special ? 21 : 20) * 8;
                const size_t offset = reader.number(entry, 4), length = reader.number(entry + 4, 4);
                if (length != 2052 || !reader.extent(offset, length) || reader.number(offset) != 63 || reader.number(offset + 2) != 31)
                {
                    return Result<bool>::failure(make_error("Invalid448984 original foot bitmap"));
                }
                foot.assign(data.begin() + offset + 4, data.begin() + offset + 2052);
            }
            const int displacement = int(int8_t(prg_.at(bitmap.frame + 6)));
            auto shadow = NativeInvisibility::shadow(scratch, bitmap.b1, bitmap.w, bitmap.h, displacement,
                foot, palette, special, pose.unit.movement_type == 9);
            if (!shadow) { return shadow; }
            auto composed = NativeInvisibility::compose(scratch, bitmap.b1, pose.unit, palette);
            if (!composed) { return composed; }
            mask_unit_ground(scratch, bitmap.w, bitmap.h, pose.unit.x, pose.unit.y, int(prg_.at(cell)), checker_mask_, displacement);
            // 4465B4(mode1) and 4462B6 erase both armies' nearer raw AD masks.
            for (const auto& other : units)
            {
                if (!other.present(turn) || !(other.status_flags & 0x80) || (other.status_flags & 0x20) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5) { continue; }
                const auto found = unit_bitmaps_.find(unsigned(&other - units.data()));
                if (found == unit_bitmaps_.end()) { continue; }
                const auto& occluder = found->second;
                if (occluder.x < bitmap.x - 2 - (displacement < 0) || occluder.x > bitmap.x + 2 + (displacement > 0) || occluder.y <= bitmap.y || occluder.y > bitmap.y + 24) { continue; }
                auto erased = s2ne::try_erase(scratch, bitmap.w, bitmap.h, occluder.ad, occluder.w, occluder.h,
                    occluder.left - bitmap.left - displacement, occluder.top - bitmap.top);
                if (!erased) { return erased; }
            }
            pose.pixels = std::move(scratch);
        }
        if (!silhouette && (pose.unit.action_state == 4 || pose.unit.movement_pose) && pose.unit.movement_visual_class >= 3 && pose.unit.movement_visual_class <= 5)
        {
            const int elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(pose.unit.y) * width + pose.unit.x) * 5);
            const int left = pose.unit.x * 64 + (pose.unit.y & 1) * 32 - camera_x;
            const int top = pose.unit.y * 16 - (elevation - 1) * 16 - camera_y;
            std::vector<uint8_t> background(2048);
            for (int y = 0; y < 32; ++y)
            {
                for (int x = 0; x < 64; ++x)
                {
                    if (left + x >= 0 && left + x < 640 && top + y >= 0 && top + y < 480)
                    {
                        background[size_t(y) * 64 + x] = movement_indices[size_t(top + y) * 640 + left + x];
                    }
                }
            }
            auto resources = try_movement_shadow(pose.unit.x, pose.unit.y);
            if (!resources) { return Result<bool>::failure(resources.error()); }
            auto shadow = s2ne::try_shadow(background, reaction_foot_mask_, reaction_shadow_);
            if (!shadow) { return Result<bool>::failure(shadow.error()); }
            for (const auto& other : presented)
            {
                if (other.unit.action_state == 1 || other.unit.action_state == 3 || other.unit.action_state == 5 || std::abs(other.unit.x - pose.unit.x) > 2 || other.unit.y <= pose.unit.y || other.unit.y > pose.unit.y + 24) { continue; }
                // 4468DF consumes raw AD, not the ground-masked display copy.
                const auto& source = unit_bitmaps_.at(other.instance);
                const int source_elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(other.unit.y) * width + other.unit.x) * 5);
                auto erased = s2ne::try_erase(shadow.value(), 64, 32, source.ad, source.w, source.h,
                    other.unit.x * 64 + (other.unit.y & 1) * 32 + 32 - int(source.w / 2) - camera_x - left,
                    other.unit.y * 16 + 32 - int(source.h) - (source_elevation - 1) * 16 - camera_y - top);
                if (!erased) { return Result<bool>::failure(erased.error()); }
            }
            mask_unit_ground(shadow.value(), 64, 32, pose.unit.x, pose.unit.y, elevation);
            for (int y = 0; y < 32; ++y)
            {
                for (int x = 0; x < 64; ++x)
                {
                    unsigned c = shadow.value()[size_t(y) * 64 + x];
                    if (!c || left + x < 0 || left + x >= 640 || top + y < 0 || top + y >= 480) { continue; }
                    size_t dst = size_t(top + y) * 640 + left + x;
                    movement_indices[dst] = uint8_t(c);
                    if (indexed) { indexed->at(dst) = uint8_t(c); }
                    for (unsigned k = 0; k < 3; ++k) { image[dst * 4 + k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63); }
                }
            }
            if (pose.unit.movement_visual_class >= 4)
            {
                auto effect = try_movement_effect(pose.unit.movement_visual_class, staged_effect_clock.phase(), pose.unit.direction);
                if (!effect) { return Result<bool>::failure(effect.error()); }
                if (pose.unit.movement_visual_class == 4)
                {
                    pending_effect_presentations_.push_back({ pose.instance, pose.unit.movement_camera_pose, elapsed_ms });
                    staged_effect_clock.presented(pose.unit.movement_camera_pose, elapsed_ms, staged_effect_stamps[pose.instance]);
                }
                const auto& bytes = effect.value();
                unsigned ew = word(bytes, 0) + 1, eh = word(bytes, 2) + 1;
                int displacement = pose.left - (pose.unit.x * 64 + (pose.unit.y & 1) * 32 + 32 - int(pose.w / 2) - camera_x);
                int ex = left - (int(ew) - 64) / 2 + displacement;
                int ey = top + 32 - int(eh) - ((pose.unit.status_flags & 4) ? 48 : 64);
                for (unsigned y = 0; y < eh; ++y)
                {
                    for (unsigned x = 0; x < ew; ++x)
                    {
                        unsigned c = bytes[4 + size_t(y) * ew + x];
                        int dx = ex + int(x), dy = ey + int(y);
                        if (!c || dx < 0 || dx >= 640 || dy < 0 || dy >= 480) { continue; }
                        size_t dst = size_t(dy) * 640 + dx;
                        movement_indices[dst] = uint8_t(c);
                        if (indexed) { indexed->at(dst) = uint8_t(c); }
                        for (unsigned k = 0; k < 3; ++k) { image[dst * 4 + k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63); }
                    }
                }
            }
            // 42B1D2 arg0=4 raises the raw AD, not the already masked ordinary pose.
            pose.pixels = unit_bitmaps_.at(pose.instance).ad;
            pose.top -= 64;
        }
        if (disappearance_.active() && pose.instance == disappearance_.target && !silhouette)
        {
            pose.pixels = disappearance_frames_.at(disappearance_.frame);
        }
        if (reaction && pose.instance == *reaction && !silhouette)
        {
            auto& bitmap = unit_bitmaps_.at(*reaction);
            auto reaction_result = try_compose_reaction(bitmap.w, bitmap.h, bitmap.ad, bitmap.b1, bitmap.alternate);
            if (!reaction_result)
            {
                return Result<bool>::failure(reaction_result.error());
            }
            pose.pixels = std::move(reaction_result).value();
            last_reaction_target_ = *reaction;
            last_reaction_left_ = bitmap.alternate;
            bitmap.alternate = !bitmap.alternate;
            last_reaction_ = pose.pixels;
            ++reaction_draws_;
            int displacement = pose.left - (pose.unit.x * 64 + (pose.unit.y & 1) * 32 + 32 - int(pose.w / 2) - camera_x);
            pose.w = 128;
            pose.left = pose.unit.x * 64 + (pose.unit.y & 1) * 32 + 32 - 64 + displacement - camera_x;
            mask_unit_ground(pose.pixels, pose.w, pose.h, pose.unit.x, pose.unit.y, int(prg_.at(item(6, map_id - 1) + 28 + (size_t(pose.unit.y) * width + pose.unit.x) * 5)), checker_mask_, displacement);
        }
        else if (!reaction && !silhouette)
        {
            unit_bitmaps_.at(pose.instance).alternate = true;
        }
        const bool external_pose = texture_resources.active() && !silhouette && !reaction && !disappearance_.active()
            && !arrival_target_ && !summon_target_ && !hidden && !(pose.unit.status_flags & 0x10)
            && pose.unit.action_state != 2 && !write_invisibility_world_;
        bool external_drawn = false;
        if (external_pose)
        {
            if (!external_units_available)
            {
                auto available = texture_resources.group_available("units");
                if (!available) { return Result<bool>::failure(available.error()); }
                external_units_available = available.value();
            }
            if (*external_units_available)
            {
                const uint64_t pose_time = pose.unit.movement_pose ? uint64_t(pose.unit.movement_frame) * 190 : idle_pose_time_.value_or(elapsed_ms);
                auto drawn = texture_resources.draw_unit(image, pose.unit.appearance, pose.unit.animation_action, pose.unit.direction & 3, pose_time,
                    pose.left, pose.top, pose.w, pose.h, pose.pixels);
                if (!drawn) { return Result<bool>::failure(drawn.error()); }
                external_drawn = true;
            }
        }
        for (unsigned y = 0; y < pose.h; ++y)
        {
            for (unsigned x = 0; x < pose.w; ++x)
            {
                unsigned c = pose.pixels.at(size_t(y) * pose.w + x);
                int dx = pose.left + int(x), dy = pose.top + int(y);
                if (write_invisibility_world_ && c && pose.unit.action_state != 1 && pose.unit.action_state != 3 && pose.unit.action_state != 5)
                {
                    const auto& bitmap = unit_bitmaps_.at(pose.instance);
                    const int wx = bitmap.left + int(x), wy = bitmap.top + int(y);
                    if (wx >= 0 && wy >= 0 && wx < int(width * 64) && wy < int(height * 32))
                    {
                        invisibility_world_.at(size_t(wy) * width * 64 + wx) = uint8_t(c);
                    }
                }
                if (!c || dx < 0 || dy < 0 || dx >= 640 || dy >= 480)
                {
                    continue;
                }
                size_t dst = (size_t(dy) * 640 + dx) * 4;
                if (!movement_indices.empty())
                {
                    movement_indices[dst / 4] = uint8_t(c);
                }
                if (indexed)
                {
                    indexed->at(dst / 4) = uint8_t(c);
                }
                if (external_drawn) { continue; }
                for (unsigned k = 0; k < 3; ++k)
                {
                    image.at(dst + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
                }
            }
        }
    }
    return Result<bool>::success(true);
}

auto OriginalMap::try_draw_units(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, unsigned turn, uint64_t elapsed_ms, std::optional<unsigned> silhouette, std::optional<unsigned> reaction, unsigned background_mode, std::optional<unsigned> only_instance, std::vector<uint8_t>* indexed) const -> Result<bool>
{
    if (image.size() != 640u * 480 * 4 || (indexed && indexed->size() != 640u * 480))
    {
        return Result<bool>::failure(make_error("Invalid unit drawing framebuffer"));
    }
    auto pending_effects = pending_effect_presentations_;
    if (!silhouette) { pending_effect_presentations_.clear(); }
    auto bitmaps = unit_bitmaps_;
    auto foot = reaction_foot_mask_;
    auto shadow = reaction_shadow_;
    auto last = last_reaction_;
    auto last_target = last_reaction_target_;
    auto last_left = last_reaction_left_;
    auto draws = reaction_draws_;
    auto output = image;
    auto indices = indexed ? *indexed : std::vector<uint8_t>{};
    auto result = draw_units_validated(output, units, turn, elapsed_ms, silhouette, reaction, background_mode, only_instance, indexed ? &indices : nullptr);
    if (!result)
    {
        pending_effect_presentations_ = std::move(pending_effects);
        unit_bitmaps_ = std::move(bitmaps);
        reaction_foot_mask_ = std::move(foot);
        reaction_shadow_ = shadow;
        last_reaction_ = std::move(last);
        last_reaction_target_ = last_target;
        last_reaction_left_ = last_left;
        reaction_draws_ = draws;
        return result;
    }
    image.swap(output);
    if (indexed)
    {
        indexed->swap(indices);
    }
    return result;
}

auto OriginalMap::draw_units(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, unsigned turn, uint64_t elapsed_ms, std::optional<unsigned> silhouette, std::optional<unsigned> reaction, unsigned background_mode, std::optional<unsigned> only_instance, std::vector<uint8_t>* indexed) const -> void
{
    auto result = try_draw_units(image, units, turn, elapsed_ms, silhouette, reaction, background_mode, only_instance, indexed);
    if (!result)
    {
        report_error(result.error());
    }
}

auto OriginalMap::prepare_invisibility_buffers(const std::vector<OriginalEvents::Unit>& baseline,
    const std::vector<OriginalEvents::Unit>& staged, const std::vector<size_t>& targets, unsigned turn, uint64_t elapsed, bool wide) -> Result<bool>
{
    if (baseline.size() != staged.size()) { return Result<bool>::failure(make_error("466D83 roster extent changed")); }
    bool newly_hidden = false;
    for (const auto target : targets)
    {
        if (target >= staged.size()) { return Result<bool>::failure(make_error("466D83 invalid target instance")); }
        if (!(baseline[target].status3c & 0x10) && (staged[target].status3c & 0x10)) { newly_hidden = true; }
    }
    if (!newly_hidden) { return Result<bool>::success(true); }
    auto prepared = *this;
    if (wide)
    {
        prepared.invisibility_side_presentations_.clear();
        for (int y = 0; y < screen_height; y += 480)
        {
            for (int x = 0; x < screen_width; x += 640)
            {
                auto slice = *this;
                slice.camera_x += x - ui_offset_x();
                slice.camera_y += y - ui_offset_y();
                auto result = slice.prepare_invisibility_buffers(baseline, staged, targets, turn, elapsed, false);
                if (!result) { return result; }
                prepared.invisibility_side_presentations_.push_back(std::move(slice.invisibility_presentations_));
            }
        }
    }
    prepared.invisibility_world_ = pixels_;
    prepared.invisibility_present_pending_ = false;
    prepared.invisibility_presentations_.clear();
    prepared.invisibility_presentation_cursor_ = 0;
    prepared.invisibility_scratch_ = 0;
    prepared.invisibility_viewports_[0].resize(640 * 480);
    for (unsigned y = 0; y < 480; ++y)
    {
        for (unsigned x = 0; x < 640; ++x)
        {
            prepared.invisibility_viewports_[0][size_t(y) * 640 + x] = viewport_pixel(pixels_, int(x), int(y));
        }
    }
    auto image = prepared.draw();
    prepared.write_invisibility_world_ = true;
    auto result = prepared.try_draw_units(image, baseline, turn, elapsed, std::nullopt, std::nullopt, 0,
        std::nullopt, &prepared.invisibility_viewports_[0]);
    if (!result) { return result; }
    prepared.invisibility_viewports_[1] = prepared.invisibility_viewports_[0];
    auto current = baseline;
    for (const auto target : targets)
    {
        if (target >= staged.size()) { return Result<bool>::failure(make_error("466D83 invalid target instance")); }
        // 442CD5 returns one only when the bit was newly installed.
        if ((baseline[target].status3c & 0x10) || !(staged[target].status3c & 0x10)) { continue; }
        current[target] = staged[target];
        prepared.invisibility_scratch_ = 1;
        result = prepared.try_draw_units(image, current, turn, elapsed, std::nullopt, std::nullopt, 0,
            unsigned(target), &prepared.invisibility_viewports_[1]);
        if (!result) { return result; }
        prepared.invisibility_scratch_ = 0;
        prepared.invisibility_present_pending_ = true;
        prepared.invisibility_presentations_.push_back(prepared.invisibility_viewports_[1]);
    }
    prepared.write_invisibility_world_ = false;
    *this = std::move(prepared);
    return Result<bool>::success(true);
}

auto OriginalMap::invisibility_world() const -> const std::vector<uint8_t>& { return invisibility_world_; }

auto OriginalMap::refresh_world(const std::vector<OriginalEvents::Unit>& roster, unsigned turn, uint64_t elapsed) -> Result<bool>
{
    auto prepared = *this;
    auto result = prepared.refresh_world_candidate(roster, turn, elapsed);
    if (!result) { return result; }
    *this = std::move(prepared);
    return Result<bool>::success(true);
}

auto OriginalMap::refresh_world_candidate(const std::vector<OriginalEvents::Unit>& roster, unsigned turn, uint64_t elapsed, const std::vector<uint8_t>* background) -> Result<bool>
{
    invisibility_world_ = pixels_;
    read_world_background_ = true;
    auto image = background ? *background : draw();
    pending_effect_presentations_.clear();
    auto result = draw_units_validated(image, roster, turn, elapsed);
    if (!result) { return result; }
    write_invisibility_world_ = true;
    pending_effect_presentations_.clear();
    result = draw_units_validated(image, roster, turn, elapsed);
    if (!result) { return result; }
    write_invisibility_world_ = false;
    read_world_background_ = false;
    return Result<bool>::success(true);
}

auto OriginalMap::world_viewport() const -> Result<std::vector<uint8_t>>
{
    if (invisibility_world_.size() != pixels_.size()) { return Result<std::vector<uint8_t>>::failure(make_error("42927B world not initialized")); }
    std::vector<uint8_t> indices(640 * 480);
    for (unsigned y = 0; y < 480; ++y)
    {
        for (int x = 0; x < 640; ++x) { indices[size_t(y) * 640 + x] = viewport_pixel(invisibility_world_, x, int(y)); }
    }
    return Result<std::vector<uint8_t>>::success(std::move(indices));
}

auto OriginalMap::try_draw_world(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& roster, unsigned turn, uint64_t elapsed, bool contains_background) const -> Result<bool>
{
    if (image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("42927B invalid framebuffer extent")); }
    auto prepared = *this;
    auto result = prepared.refresh_world_candidate(roster, turn, elapsed, contains_background ? &image : nullptr);
    if (!result) { return result; }
    if (texture_resources.active())
    {
        auto output = contains_background ? image : draw();
        invisibility_world_ = std::move(prepared.invisibility_world_);
        unit_bitmaps_ = std::move(prepared.unit_bitmaps_);
        image.swap(output);
        return Result<bool>::success(true);
    }
    auto cropped = prepared.world_viewport();
    if (!cropped) { return Result<bool>::failure(cropped.error()); }
    auto output = image;
    for (size_t p = 0; p < cropped.value().size(); ++p)
    {
        for (unsigned c = 0; c < 3; ++c) { output[p * 4 + c] = uint8_t(unsigned(pal_.at(cropped.value()[p] * 3 + c)) * 255 / 63); }
        output[p * 4 + 3] = 255;
    }
    // 447FC3 restores B1 without its viewport-only signed AD displacement.
    // The caller then applies4462B6 after selection overlays.
    for (const auto& [instance, bitmap] : prepared.unit_bitmaps_)
    {
        if (instance >= roster.size()) { continue; }
        const auto& unit = roster[instance];
        if (!unit.present(turn) || (unit.status_flags & 0x20) || !(unit.status_flags & 0x80) || unit.action_state == 1 || unit.action_state == 3 || unit.action_state == 5) { continue; }
        for (unsigned y = 0; y < bitmap.h; ++y)
        {
            for (unsigned x = 0; x < bitmap.w; ++x)
            {
                const int dx = bitmap.left + int(x) - camera_x, dy = bitmap.top + int(y) - camera_y;
                if (dx < 0 || dx >= 640 || dy < 0 || dy >= 480) { continue; }
                const auto ink = bitmap.b1.at(size_t(y) * bitmap.w + x);
                const size_t p = (size_t(dy) * 640 + dx) * 4;
                for (unsigned c = 0; c < 3; ++c) { output[p + c] = uint8_t(unsigned(pal_.at(ink * 3 + c)) * 255 / 63); }
            }
        }
    }
    invisibility_world_ = std::move(prepared.invisibility_world_);
    unit_bitmaps_ = std::move(prepared.unit_bitmaps_);
    image.swap(output);
    return Result<bool>::success(true);
}

auto OriginalMap::refresh_army_world(const std::vector<OriginalEvents::Unit>& roster, bool hostile, unsigned turn, uint64_t elapsed) -> Result<bool>
{
    auto prepared = *this;
    const bool initialize = prepared.invisibility_world_.empty();
    if (initialize) { prepared.invisibility_world_ = pixels_; }
    auto image = prepared.draw();
    // Warm both armies' AD/B1 before4466FC visits just the opposing army.
    auto result = prepared.try_draw_units(image, roster, turn, elapsed);
    if (!result) { return result; }
    prepared.write_invisibility_world_ = true;
    if (initialize)
    {
        result = prepared.try_draw_units(image, roster, turn, elapsed);
        if (!result) { return result; }
    }
    for (unsigned i = 0; i < roster.size(); ++i)
    {
        const auto& unit = roster[i];
        if (unit.hostile() != hostile || unit.action_state == 1 || unit.action_state == 3 || unit.action_state == 5) { continue; }
        result = prepared.try_draw_units(image, roster, turn, elapsed, std::nullopt, std::nullopt, 0, i);
        if (!result) { return result; }
    }
    prepared.write_invisibility_world_ = false;
    *this = std::move(prepared);
    return Result<bool>::success(true);
}

auto OriginalMap::invisibility_viewport(unsigned index) const -> const std::vector<uint8_t>& { return invisibility_viewports_.at(index); }

auto OriginalMap::invisibility_scratch() const -> unsigned { return invisibility_scratch_; }

auto OriginalMap::invisibility_present_pending() const -> bool { return invisibility_present_pending_; }

auto OriginalMap::invisibility_presented() -> void
{
    if (!invisibility_present_pending_) { return; }
    ++invisibility_presentation_cursor_;
    if (invisibility_presentation_cursor_ == invisibility_presentations_.size())
    {
        invisibility_present_pending_ = false;
        invisibility_presentations_.clear();
        for (auto& side : invisibility_side_presentations_) { side.clear(); }
        invisibility_presentation_cursor_ = 0;
    }
}

auto OriginalMap::present_invisibility_viewport(std::optional<unsigned> side) const -> std::vector<uint8_t>
{
    std::vector<uint8_t> image(640 * 480 * 4, 255);
    const auto& presentations = side ? invisibility_side_presentations_.at(*side) : invisibility_presentations_;
    const auto& viewport = presentations.at(invisibility_presentation_cursor_);
    for (size_t p = 0; p < viewport.size(); ++p)
    {
        for (unsigned c = 0; c < 3; ++c) { image[p * 4 + c] = uint8_t(unsigned(pal_.at(viewport[p] * 3 + c)) * 255 / 63); }
    }
    return image;
}

auto OriginalMap::skill32_background(const std::vector<OriginalEvents::Unit>& units, unsigned turn, unsigned caster, unsigned target, unsigned tint_source, unsigned tint_amount) const -> Result<std::vector<uint8_t>>
{
    auto image = draw();
    std::vector<uint8_t> indexed(640 * 480);
    for (unsigned y = 0; y < 480; ++y)
    {
        for (unsigned x = 0; x < 640; ++x)
        {
            indexed[size_t(y) * 640 + x] = viewport_pixel(pixels_, int(x), int(y));
        }
    }
    auto drawn = try_draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, std::nullopt, &indexed);
    if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
    const auto& palette = *palette_;
    for (auto& color : indexed)
    {
        if (color)
        {
            color = uint8_t(palette.blend_index(color, tint_source, tint_amount));
        }
    }
    for (size_t p = 0; p < indexed.size(); ++p)
    {
        for (unsigned k = 0; k < 3; ++k)
        {
            image[p * 4 + k] = uint8_t(unsigned(pal_.at(indexed[p] * 3 + k)) * 255 / 63);
        }
    }
    if (tint_amount == 0) { skill88_indexed_ = indexed; }
    // 46B5DE restores the caster and the selected enemy after the indexed tint.
    auto restored = try_draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, caster);
    if (!restored) { return Result<std::vector<uint8_t>>::failure(restored.error()); }
    restored = try_draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, target);
    if (!restored) { return Result<std::vector<uint8_t>>::failure(restored.error()); }
    return Result<std::vector<uint8_t>>::success(std::move(image));
}

auto OriginalMap::skill32_frame(const std::vector<uint8_t>& background, const OriginalSkill32Visual::Frame& frame, const std::vector<OriginalEvents::Unit>& units, unsigned turn, unsigned target, const OriginalSkill32Visual* visual) const -> std::vector<uint8_t>
{
    const auto& destination = units.at(target);
    int x = destination.x, y = destination.y;
    size_t cells = item(6, map_id - 1) + 28;
    int elevation = prg_.at(cells + (size_t(y) * width + x) * 5);
    auto pixels = frame.pixels;
    mask_unit_ground(pixels, frame.width, frame.height, x, y, elevation);
    int left = x * 64 + (y & 1) * 32 + 32 - int(frame.width / 2), top = y * 16 + 32 - int(frame.height);
    // 46B9A9 mode2 -> 4468DF(arg0=1,arg4=0): clear nearer unit AD silhouettes.
    for (unsigned i = 0; i < units.size(); ++i)
    {
        const auto& u = units[i];
        if (!u.present(turn) || (u.status_flags & 0x20) || !(u.status_flags & 0x80) || u.action_state == 1 || u.action_state == 3 || u.action_state == 5 || (u.x == x && u.y == y) || u.x < x - 2 || u.x > x + 2 || u.y <= y || u.y > y + 24)
        {
            continue;
        }
        auto found = unit_bitmaps_.find(i);
        if (found == unit_bitmaps_.end())
        {
            continue;
        }
        int rise = prg_.at(cells + (size_t(u.y) * width + u.x) * 5);
        if (!rise)
        {
            continue;
        }
        const auto& b = found->second;
        int dx = u.x * 64 + (u.y & 1) * 32 + 32 - int(b.w / 2) - left, dy = u.y * 16 + 32 - int(b.h) - top - (rise - elevation) * 16;
        for (unsigned yy = 0; yy < b.h; ++yy)
        {
            for (unsigned xx = 0; xx < b.w; ++xx)
            {
                int tx = dx + int(xx), ty = dy + int(yy);
                if (tx >= 0 && ty >= 0 && tx < int(frame.width) && ty < int(frame.height) && b.ad.at(size_t(yy) * b.w + xx))
                {
                    pixels.at(size_t(ty) * frame.width + tx) = 0;
                }
            }
        }
    }
    auto image = background;
    const auto& palette = *palette_;
    top -= camera_y + std::max(0, elevation - 1) * 16;
    left -= camera_x;
    if (texture_resources.active())
    {
        auto drawn = texture_resources.draw_masked(image, "spells/" + std::to_string(frame.texture_visual), frame.texture_frame, left, top, frame.width, frame.height, pixels);
        if (drawn) { return image; }
    }
    std::vector<int16_t> blended;
    if (visual) { blended.assign(256 * 256, -1); }
    for (unsigned yy = 0; yy < frame.height; ++yy)
    {
        for (unsigned xx = 0; xx < frame.width; ++xx)
        {
            auto c = pixels.at(size_t(yy) * frame.width + xx);
            int dx = left + int(xx), dy = top + int(yy);
            if (!c || dx < 0 || dy < 0 || dx >= 640 || dy >= 480)
            {
                continue;
            }
            if (visual && visual->amount(c) >= 0)
            {
                size_t p = (size_t(dy) * 640 + dx) * 4;
                unsigned destinationColor = skill88_indexed_.at(p / 4);
                auto& cached = blended[destinationColor * 256 + c];
                if (cached < 0) { cached = palette.blend_index(destinationColor, c, visual->amount(c)); }
                c = uint8_t(cached);
            }
            for (unsigned k = 0; k < 3; ++k)
            {
                image.at((size_t(dy) * 640 + dx) * 4 + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
            }
        }
    }
    return image;
}

auto OriginalMap::skill88_background(const std::vector<OriginalEvents::Unit>& units, unsigned turn, unsigned caster, unsigned tint_amount, unsigned tint_source, std::optional<unsigned> partner) const -> Result<std::vector<uint8_t>>
{
    auto image = draw();
    std::vector<uint8_t> indexed(640 * 480);
    for (unsigned y = 0; y < 480; ++y)
    {
        for (unsigned x = 0; x < 640; ++x)
        {
            indexed[size_t(y) * 640 + x] = viewport_pixel(pixels_, int(x), int(y));
        }
    }
    auto drawn = try_draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, std::nullopt, &indexed);
    if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
    const auto& palette = *palette_;
    std::array<uint8_t, 256> tint{};
    for (unsigned c = 0; c < tint.size(); ++c) { tint[c] = c ? palette.blend_index(c, tint_source, tint_amount) : 0; }
    for (size_t p = 0; p < indexed.size(); ++p)
    {
        unsigned c = tint[indexed[p]];
        indexed[p] = uint8_t(c);
        for (unsigned k = 0; k < 3; ++k)
        {
            image[p * 4 + k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
        }
    }
    auto restored = try_draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, caster, &indexed);
    if (!restored) { return Result<std::vector<uint8_t>>::failure(restored.error()); }
    if (partner)
    {
        restored = try_draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, *partner, &indexed);
        if (!restored) { return Result<std::vector<uint8_t>>::failure(restored.error()); }
    }
    skill88_indexed_ = std::move(indexed);
    return Result<std::vector<uint8_t>>::success(std::move(image));
}

auto OriginalMap::skill88_frame(const std::vector<uint8_t>& background, const OriginalSkill88Visual& visual, const OriginalEvents::Unit& actor, std::optional<size_t> frame_index, const std::vector<OriginalEvents::Unit>* occluders, unsigned turn) const -> std::vector<uint8_t>
{
    auto image = background;
    const auto& frame = frame_index ? visual.frame_at(*frame_index) : visual.current();
    auto pixels = frame.pixels;
    if (occluders)
    {
        const auto cells = item(6, map_id - 1) + 28;
        const int height = prg_.at(cells + (size_t(actor.y) * width + actor.x) * 5);
        mask_unit_ground(pixels, frame.width, frame.height, actor.x, actor.y, height);
        for (unsigned i = 0; i < occluders->size(); ++i)
        {
            const auto& unit = occluders->at(i);
            if (!unit.present(turn) || !(unit.status_flags & 0x80) || (unit.status_flags & 0x20) || unit.action_state == 1 || unit.action_state == 3 || unit.action_state == 5 || std::abs(unit.x - actor.x) > 2 || unit.y <= actor.y || unit.y > actor.y + 24) { continue; }
            const auto found = unit_bitmaps_.find(i);
            if (found == unit_bitmaps_.end()) { continue; }
            const auto& bitmap = found->second;
            const int elevation = prg_.at(cells + (size_t(unit.y) * width + unit.x) * 5);
            const int dx = unit.x * 64 + (unit.y & 1) * 32 + 32 - int(bitmap.w / 2) - (actor.x * 64 + (actor.y & 1) * 32 + 32 - int(frame.width / 2));
            const int dy = unit.y * 16 + 32 - int(bitmap.h) - (actor.y * 16 + 32 - int(frame.height)) - (elevation - height) * 16;
            auto erased = s2ne::try_erase(pixels, frame.width, frame.height, bitmap.ad, bitmap.w, bitmap.h, dx, dy);
            if (!erased)
            {
                report_error(erased.error());
                return background;
            }
        }
    }
    const auto& palette = *palette_;
    int left = actor.x * 64 + (actor.y & 1) * 32 - camera_x + 32 - visual.anchor_x(), top = actor.y * 16 - camera_y + 32 - visual.anchor_y();
    if (visual.anchor_x() == -1) { left = 0; }
    if (visual.anchor_y() == -1) { top = 0; }
    int elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(actor.y) * width + actor.x) * 5);
    if (visual.anchor_y() != -1) { top -= std::max(0, elevation - 1) * 16; }
    if (texture_resources.active())
    {
        auto drawn = texture_resources.draw_masked(image, "spells/" + std::to_string(visual.texture_visual()), unsigned(visual.texture_frame(frame_index.value_or(visual.frame()))), left, top, frame.width, frame.height, pixels);
        if (drawn) { return image; }
    }
    std::vector<int16_t> blended(256 * 256, -1);
    for (unsigned y = 0; y < frame.height; ++y)
    {
        for (unsigned x = 0; x < frame.width; ++x)
        {
            unsigned c = pixels[size_t(y) * frame.width + x];
            int dx = left + int(x), dy = top + int(y);
            if (!c || dx < 0 || dy < 0 || dx >= 640 || dy >= 480)
            {
                continue;
            }
            size_t p = (size_t(dy) * 640 + dx) * 4;
            if (visual.amount(c) >= 0)
            {
                const unsigned source = skill88_indexed_.at(p / 4);
                auto& cached = blended[source * 256 + c];
                if (cached < 0) { cached = palette.blend_index(source, c, visual.amount(c)); }
                c = unsigned(cached);
            }
            for (unsigned k = 0; k < 3; ++k)
            {
                image[p + k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
            }
        }
    }
    return image;
}

auto OriginalMap::constrained_camera(int x, int y) const -> std::pair<int, int>
{
    const int original_right = std::max(32, int(width * 64) - 21 * 32);
    int left = 32, right = original_right;
    // 原交错地表两边各留半列；只有地图能容纳全宽时才收紧中央镜头。
    if (original_right - 32 >= viewport_extension_ * 2)
    {
        left += viewport_extension_;
        right -= viewport_extension_;
    }
    int top = 16, bottom = std::max(16, int(height * 32) - 480);
    if (bottom - top >= viewport_extension_y_ * 2)
    {
        top += viewport_extension_y_;
        bottom -= viewport_extension_y_;
    }
    return { std::clamp(x, left, right), std::clamp(y, top, bottom) };
}

auto OriginalMap::constrain_camera() -> void
{
    const auto position = constrained_camera(camera_x, camera_y);
    camera_x = position.first;
    camera_y = position.second;
}

auto OriginalMap::set_viewport_extension(int extension, int vertical) -> void
{
    viewport_extension_ = (std::max(0, extension) + 31) / 32 * 32;
    viewport_extension_y_ = (std::max(0, vertical) + 15) / 16 * 16;
    constrain_camera();
}

auto OriginalMap::pan(int dx, int dy) -> void
{
    const auto position = constrained_camera(camera_x + dx, camera_y + dy);
    camera_x = position.first;
    camera_y = position.second;
}

auto OriginalMap::edge_scroll(float x, float y) -> bool
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480)
    {
        return false;
    }
    // 4307E8: strict ten-pixel edges; 42BACD: one half-column / two rows.
    int before_x = camera_x, before_y = camera_y;
    int panX;
    if (x < 10) { panX = -32; }
    else if (x > 630) { panX = 32; }
    else
    {
        panX = 0;
    }
    int panY = 0;
    if (y < 10) { panY = -32; }
    else if (y > 470) { panY = 32; }
    pan(panX, panY);
    return camera_x != before_x || camera_y != before_y;
}

auto OriginalMap::evacuation_visible(int x, int y) const -> bool
{
    if (y < 1 || y >= int(height * 2) - 1 || x < 0 || x >= int(width) - (y & 1))
    {
        return false;
    }
    int half = camera_x / 32, left = half / 2;
    if (half & 1)
    {
        if (x < left || x > left + 10 || ((y & 1) && x == left + 10))
        {
            return false;
        }
    }
    else if (x < left - ((y & 1) ? 1 : 0) || x >= left + 10)
    {
        return false;
    }
    int elevation = int(prg_.at(item(6, map_id - 1) + 28 + (size_t(y) * width + x) * 5));
    int row = y - std::max(0, elevation - 1), top = camera_y / 16;
    return row >= top + 2 && row < top + 30;
}

auto OriginalMap::evacuation_camera_step(int x, int y) -> bool
{
    int elevation = int(prg_.at(item(6, map_id - 1) + 28 + (size_t(y) * width + x) * 5));
    int hx = std::max(1, std::min(2 * x - ((y & 1) ? 8 : 9), int(width) * 2 - 21));
    int vy = std::max(1, std::min(y - (elevation - 1 + 16), int(height) * 2 - 30));
    const auto target = constrained_camera(hx * 32, vy * 16);
    hx = target.first / 32;
    vy = target.second / 16;
    int cx = camera_x / 32, cy = camera_y / 16;
    cx += (cx < hx) - (cx > hx);
    cy += (cy < vy) - (cy > vy);
    camera_x = cx * 32;
    camera_y = cy * 16;
    constrain_camera();
    return cx == hx && cy == vy;
}

auto OriginalMap::center(int x, int y) -> void
{
    // 原42B37A以半列/行定位并保留左侧一半列，不能从像素零开始暴露菱形边缘。
    movement_camera(x, y);
}

auto OriginalMap::draw() const -> std::vector<uint8_t>
{
    std::vector<uint8_t> image(640 * 480 * 4);
    if (texture_resources.map_available())
    {
        for (size_t p = 0; p < image.size(); p += 4)
        {
            for (unsigned k = 0; k < 3; ++k) { image[p + k] = uint8_t(unsigned(pal_.at(k)) * 255 / 63); }
            image[p + 3] = 255;
        }
        auto started = texture_resources.begin_canvas(image);
        if (!started) { fatal(started.error()); }
        std::optional<Error> draw_error;
        for (const auto& placement : texture_placements_)
        {
            const auto& sprite = placement.image;
            if (placement.x + int(sprite.w) <= camera_x || placement.y + int(sprite.h) <= camera_y || placement.x >= camera_x + 640 || placement.y >= camera_y + 480) { continue; }
            if (!sprite.texture_group) { continue; }
            std::string group_name;
            switch (sprite.texture_group)
            {
            case 3: group_name = "ground"; break;
            case 4: group_name = "objects"; break;
            case 5: group_name = "scenery"; break;
            case 8: group_name = "battle-backdrops"; break;
            default: fatal("Invalid external map texture group");
            }
            std::vector<uint8_t> mask = sprite.pixels;
            if (!placement.transparent) { std::fill(mask.begin(), mask.end(), uint8_t(1)); }
            auto drawn = texture_resources.draw_masked(image, group_name, sprite.texture_id, placement.x - camera_x, placement.y - camera_y, sprite.w, sprite.h, mask);
            if (!drawn)
            {
                draw_error = drawn.error();
                break;
            }
        }
        auto completed = texture_resources.end_canvas(image);
        if (!completed) { fatal(completed.error()); }
        if (draw_error) { fatal(*draw_error); }
        return image;
    }
    std::array<std::array<uint8_t, 4>, 256> colors{};
    for (unsigned c = 0; c < colors.size(); ++c)
    {
        for (unsigned k = 0; k < 3; ++k) { colors[c][k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63); }
        colors[c][3] = 255;
    }
    for (int y = 0; y < 480; ++y)
    {
        for (int x = 0; x < 640; ++x)
        {
            unsigned c = viewport_pixel(pixels_, x, y);
            size_t dst = (size_t(y) * 640 + x) * 4;
            std::copy(colors[c].begin(), colors[c].end(), image.begin() + dst);
        }
    }
    return image;
}

auto OriginalMap::sight_background(const std::vector<OriginalEvents::Unit>& units, unsigned turn, uint64_t elapsed) const -> Result<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
{
    auto prepared = *this;
    auto result = prepared.refresh_world(units, turn, elapsed);
    if (!result) { return Result<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>::failure(result.error()); }
    auto viewport = prepared.world_viewport();
    if (!viewport) { return Result<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>::failure(viewport.error()); }
    auto indices = std::move(viewport).value();
    std::vector<uint8_t> image(640 * 480 * 4, 255);
    for (size_t p = 0; p < indices.size(); ++p)
    {
        for (unsigned c = 0; c < 3; ++c) { image[p * 4 + c] = uint8_t(unsigned(pal_.at(indices[p] * 3 + c)) * 255 / 63); }
    }
    return Result<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>::success({ std::move(image), std::move(indices) });
}

auto OriginalMap::draw(uint64_t elapsed_ms) const -> std::vector<uint8_t>
{
    (void)elapsed_ms;
    return draw();
}
}    //namespace s2ne
