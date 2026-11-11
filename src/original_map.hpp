#pragma once
#include "original_combat_visual.hpp"
#include "original_disc.hpp"
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_movement_bitmap.hpp"
#include "original_palette.hpp"
#include "original_skill88_visual.hpp"
#include "original_skill_visual.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <utility>

namespace s2ne
{
class OriginalMap
{
    std::vector<uint8_t> prg_, pal_, pixels_, terrain_names_;
    unsigned terrain_ground_names_{ }, terrain_object_names_{ };
    struct Image
    {
        unsigned w, h;
        std::vector<uint8_t> pixels;
        std::vector<uint8_t> literal;
    };
    mutable std::map<size_t, Image> cache_;
    struct UnitBitmap
    {
        unsigned w{ }, h{ };
        size_t frame{ };
        int x{ }, y{ }, left{ }, top{ };
        uint8_t state{ };
        unsigned status{ };
        std::vector<uint8_t> ad, b1;
        bool alternate = true;
    };
    mutable std::map<unsigned, UnitBitmap> unit_bitmaps_;
    std::optional<uint64_t> idle_pose_time_;
    original_movement::SharedEffectClock movement_effect_clock_;
    std::map<unsigned, uint64_t> movement_effect_stamps_;
    struct EffectPresentation
    {
        unsigned actor;
        bool camera;
        uint64_t time;
    };
    mutable std::vector<EffectPresentation> pending_effect_presentations_;
    mutable std::vector<uint8_t> skill88_indexed_;
    original_combat::Disappearance disappearance_;
    std::optional<unsigned> arrival_target_;
    unsigned arrival_frame_{ }, arrival_elapsed_{ }, arrival_color_{ 194 };
    bool arrival_departure_{ };
    bool arrival_presented_{ };
    mutable std::vector<std::vector<uint8_t>> disappearance_frames_;
    mutable uint64_t reaction_draws_{ };
    mutable unsigned last_reaction_target_{ };
    mutable bool last_reaction_left_{ };
    mutable std::vector<uint8_t> last_reaction_;
    mutable std::vector<uint8_t> reaction_foot_mask_;
    mutable std::array<uint8_t, 256> reaction_shadow_{ };
    static void overlay_bitmap(UnitBitmap& destination, const UnitBitmap& source, bool background)
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
    std::filesystem::path root_;
    OriginalDisc disc_ = OriginalDisc::one;
    bool checker_mask_ = false;
    std::array<uint8_t, 80> event_flags_{ };
    std::vector<std::array<unsigned, 4>> dynamic_object_chains_;
    bool composing_static_ = false;
    std::vector<size_t> cleared_object_cells_;
    struct DynamicObject
    {
        int x, y, left, top;
        Image background;
    };
    std::vector<DynamicObject> dynamic_objects_;
    std::vector<std::pair<int, int>> removed_dynamic_cells_;
    unsigned cell_object(size_t cell) const
    {
        return std::find(cleared_object_cells_.begin(), cleared_object_cells_.end(), cell) != cleared_object_cells_.end() ? 0 : word(prg_, cell + 3);
    }
    std::vector<unsigned> object_attributes_, scenery_attributes_;
    std::array<std::array<uint8_t, 256>, 2> object_shadows_{ };
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    size_t group(unsigned id) const { return dword(pal_, 808 + 8 * (64 + id - 1)) - 10 - (id - 1); }
    Result<size_t> try_item(unsigned archive, unsigned index) const
    {
        size_t g = group(archive);
        if (index >= word(prg_, g))
        {
            return Result<size_t>::failure(make_error("Invalid original map sprite index archive=" + std::to_string(archive) + " index=" + std::to_string(index) + " count=" + std::to_string(word(prg_, g))));
        }
        return Result<size_t>::success(g + dword(prg_, g + 2 + index * 8));
    }
    size_t item(unsigned archive, unsigned index) const { return try_item(archive, index).value(); }
    Result<Image> try_raw(size_t offset) const
    {
        if (cache_.contains(offset))
        {
            return Result<Image>::success(cache_.at(offset));
        }
        if (offset > prg_.size() || prg_.size() - offset < 4)
        {
            return Result<Image>::failure(make_error("Truncated original map sprite header"));
        }
        Image image{ word(prg_, offset) + 1, word(prg_, offset + 2) + 1, { } };
        if (image.w > 8192 || image.h > 8192)
        {
            return Result<Image>::failure(make_error("Invalid original map sprite dimensions"));
        }
        if (offset + 4 > prg_.size() || size_t(image.w) * image.h > prg_.size() - offset - 4)
        {
            return Result<Image>::failure(make_error("Truncated original map sprite"));
        }
        image.pixels.assign(prg_.begin() + offset + 4, prg_.begin() + offset + 4 + size_t(image.w) * image.h);
        cache_.emplace(offset, image);
        return Result<Image>::success(std::move(image));
    }
    const Image& raw(size_t offset) const
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
    void mask_unit_ground(std::vector<uint8_t>& pixels, unsigned w, unsigned h, int x, int y, int elevation, bool checker = false, int displacement = 0) const
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
    Result<Image> try_backdrop(unsigned id) const
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
        Image image{ word(prg_, p) + 1, word(prg_, p + 2) + 1, { } };
        p += 4;
        if (image.w > 8192 || image.h > 8192 || end > prg_.size())
        {
            return Result<Image>::failure(make_error("Invalid original map backdrop"));
        }
        image.pixels.reserve(size_t(image.w) * image.h);
        image.literal.assign(size_t(image.w) * image.h, 0);
        if (encoding)
        {
            if (size_t(image.w) * image.h > end - p)
            {
                return Result<Image>::failure(make_error("Truncated original raw backdrop"));
            }
            image.pixels.assign(prg_.begin() + p, prg_.begin() + p + size_t(image.w) * image.h);
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
                        image.pixels.push_back(0);
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
                            image.literal.at(image.pixels.size()) = 1;
                        }
                        image.pixels.push_back(uint8_t(c));
                        ++x;
                    }
                }
            }
        }
        return Result<Image>::success(std::move(image));
    }
    Image backdrop(unsigned id) const
    {
        auto result = try_backdrop(id);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    void blit(const Image& image, int x, int y, bool transparent, bool object = false)
    {
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

public:
    bool remove_dynamic_object(int x, int y)
    {
        auto found = std::find_if(dynamic_objects_.begin(), dynamic_objects_.end(), [&](const auto& object)
            {
                return object.x == x && object.y == y;
            });
        if (found == dynamic_objects_.end())
        {
            return false;
        }
        blit(found->background, found->left, found->top, false);
        cleared_object_cells_.push_back(item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5);
        dynamic_objects_.erase(found);
        removed_dynamic_cells_.emplace_back(x, y);
        return true;
    }
    Result<unsigned> try_object_at(int x, int y) const
    {
        if (x < 0 || y < 0 || unsigned(y) >= height * 2 || unsigned(x) >= width - (unsigned(y) & 1))
        {
            return Result<unsigned>::failure(make_error("Invalid original object cell"));
        }
        return Result<unsigned>::success(cell_object(item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5));
    }
    unsigned object_at(int x, int y) const
    {
        auto result = try_object_at(x, y);
        if (!result)
        {
            report_error(result.error());
            return 0;
        }
        return result.value();
    }
    unsigned dynamic_marker(int x, int y) const
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
    void mask_selection(std::vector<uint8_t>& pixels, int x, int y) const
    {
        if (pixels.empty() || pixels.size() % 64 || x < 0 || y < 0 || unsigned(y) >= height * 2 || unsigned(x) >= width - (unsigned(y) & 1)) { return; }
        const auto elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(y) * width + unsigned(x)) * 5);
        // 42AFBA -> 42F027: the range/cursor bitmap uses the same forward clipping rings.
        mask_unit_ground(pixels, 64, unsigned(pixels.size() / 64), x, y, elevation);
    }
    Result<bool> try_mask_selection_units(std::vector<uint8_t>& pixels, int x, int y, const std::vector<OriginalEvents::Unit>& units, unsigned turn, uint64_t elapsed, int layer, bool idle_cursor = false) const
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
    void set_idle_pose_time(uint64_t time) { idle_pose_time_ = time; }
    void set_checker_mask(bool enabled) { checker_mask_ = enabled; }
    void set_dynamic_object_chains(const std::vector<std::array<unsigned, 4>>& chains) { dynamic_object_chains_ = chains; }
    void set_event_flags(const std::array<uint8_t, 80>& flags)
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
    unsigned width{ }, height{ }, map_id{ };
    // 42BEED: the terrain area shares the status panel's top/bottom placement.
    void draw_terrain(const OriginalMenu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const
    {
        if (!map_id || x < 0 || y < 0 || x >= int(width) || y >= int(height * 2)) { return; }
        size_t m = item(6, map_id - 1), cell = m + 28 + (size_t(y) * width + x) * 5;
        unsigned ground = word(prg_, cell + 1), object = cell_object(cell);
        if (ground == 1) { return; }
        size_t attributes = group(7), ground_attributes = attributes + 6;
        unsigned attr = ground ? word(prg_, ground_attributes + (ground - 1) * 2) : 0;
        if (!attr)
        {
            font.draw_original_large(image, font.text_bytes(0), 543, panel_y + 50, 179);
            return;
        }
        size_t lookups = m + 28 + size_t(width) * height * 10 + word(prg_, m + 12) * 10 + word(prg_, m + 14) * 6;
        unsigned ground_used = word(prg_, lookups), ground_count = word(prg_, group(3));
        size_t ground_lookup = lookups + 2;
        size_t object_lookup = ground_lookup + ground_count * 2 + ground_used * 8 + 2;
        auto put = [&](unsigned ink, int dx, int dy)
        {
            if (!ink || dx < 0 || dx >= 640 || dy < 0 || dy >= 480) { return; }
            size_t p = (size_t(dy) * 640 + dx) * 4;
            for (unsigned c = 0; c < 3; ++c) { image.at(p + c) = uint8_t(unsigned(pal_.at(ink * 3 + c)) * 255 / 63); }
            image.at(p + 3) = 255;
        };
        auto mapped_image = [&](unsigned layer, unsigned id, size_t lookup) -> const Image&
        {
            return raw(item(layer, word(prg_, lookup + (id - 1) * 2) - 1));
        };
        const auto& ground_image = mapped_image(3, ground, ground_lookup);
        for (unsigned yy = 0; yy < ground_image.h; ++yy)
        {
            for (unsigned xx = 0; xx < ground_image.w; ++xx)
            {
                unsigned ink = ground_image.pixels.at(size_t(yy) * ground_image.w + xx);
                if ((ground == 5 || ground == 6) && !ink) { continue; }
                // 4753A4 is transparent; the panel's ground images use their original shape.
                put(ink, 526 + int(xx), panel_y + 40 + int(yy));
            }
        }
        unsigned category = object ? object_attributes_.at(object - 1) >> 8 : 0;
        bool visible_object = object && !(category == 5 && !event_flags_[1]) && !(category == 6 && !event_flags_[0]) && !(category == 11 && !event_flags_[3]);
        if (visible_object)
        {
            const auto& object_image = mapped_image(4, object, object_lookup);
            // 42C16F copies only the bottom 64x32 of the original 64x96 object image.
            for (unsigned yy = 0; yy < 32; ++yy)
            {
                for (unsigned xx = 0; xx < 64; ++xx)
                {
                    put(object_image.pixels.at(4096 + yy * 64 + xx), 526 + int(xx), panel_y + 40 + int(yy));
                }
            }
        }
        auto small = [&](const std::string& text, int dx, int dy)
        {
            font.draw_small(image, { text.begin(), text.end() }, dx, panel_y + dy, 137, true);
        };
        small("X=" + std::to_string(x), 595, 48);
        small("Y=" + std::to_string(y), 595, 64);
        font.draw_original_large(image, [&]
            {
                auto s = std::to_string(prg_.at(cell));
                return std::vector<uint8_t>(s.begin(), s.end());
            }(),
            531, panel_y + 14, 141);
        unsigned local = attr & 255;
        if (visible_object && category != 7 && category != 12)
        {
            unsigned object_local = object_attributes_.at(object - 1) & 255;
            if (object_local <= terrain_object_names_) { local = terrain_ground_names_ + object_local; }
        }
        if (!local || size_t(local) * 11 > terrain_names_.size()) { return; }
        size_t start = size_t(local - 1) * 11;
        std::vector<uint8_t> name;
        for (size_t p = start; p < start + 11 && terrain_names_[p]; ++p) { name.push_back(terrain_names_[p]); }
        font.draw_small(image, name, 596 - int(name.size()) * 4, panel_y + 17, 54, true);
        font.draw_small(image, name, 595 - int(name.size()) * 4, panel_y + 16, 178, true);
    }
    void movement_effect_presented()
    {
        for (const auto& effect : pending_effect_presentations_)
        {
            movement_effect_clock_.presented(effect.camera, effect.time, movement_effect_stamps_[effect.actor]);
        }
        pending_effect_presentations_.clear();
    }
    unsigned movement_effect_phase() const { return movement_effect_clock_.phase(); }
    Result<std::vector<uint8_t>> try_movement_effect(unsigned movement_class, unsigned frame, unsigned direction) const
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
    Result<std::vector<uint8_t>> try_movement_shadow(int x, int y) const
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
        return original_movement::try_shadow(background, reaction_foot_mask_, reaction_shadow_);
    }
    int camera_x{ }, camera_y{ };
    // Explicit indexed B1 contract: callers must supply 4483CB's reconstructed
    // background, including 4485DB/448773 roster overlays, never a framebuffer.
    Result<std::vector<uint8_t>> try_compose_reaction(unsigned w, unsigned h, std::span<const uint8_t> current, std::span<const uint8_t> background, bool left) const
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
            OriginalPalette palette(pal_);
            for (unsigned i = 0; i < 256; ++i)
            {
                reaction_shadow_[i] = palette.map(5, i);
            }
        }
        return original_combat::try_reaction_bitmap(w, h, current, background, reaction_foot_mask_, reaction_shadow_, left);
    }
    std::vector<uint8_t> compose_reaction(unsigned w, unsigned h, std::span<const uint8_t> current, std::span<const uint8_t> background, bool left) const
    {
        auto result = try_compose_reaction(w, h, current, background, left);
        if (!result)
        {
            report_error(result.error());
            return { };
        }
        return std::move(result).value();
    }
    void set_resource_domain(OriginalDisc disc)
    {
        if (disc_ != disc)
        {
            disc_ = disc;
            reaction_foot_mask_.clear();
        }
    }
    OriginalMap() = default;
    static Result<OriginalMap> create(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one)
    {
        OriginalMap result;
        result.root_ = root;
        result.disc_ = disc;
        auto prg = read_resource(root / "SUPERSP.PRG"), pal = read_resource(root / "SUPERSP.PAL");
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
        result.pal_[0] = result.pal_[1] = result.pal_[2] = 0;
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
        OriginalPalette palette(result.pal_);
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
    explicit OriginalMap(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one)
    {
        auto result = create(root, disc);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    Result<bool> try_load(unsigned id)
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
    void load(unsigned id)
    {
        auto result = try_load(id);
        if (!result)
        {
            fatal(result.error());
        }
    }
    Result<bool> load_validated(unsigned id)
    {
        unit_bitmaps_.clear();
        pending_effect_presentations_.clear();
        movement_effect_stamps_.clear();
        last_reaction_.clear();
        reaction_draws_ = 0;
        map_id = id;
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
                    mask_unit_ground(image.pixels, image.w, image.h, x, row, level);
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
                    Image background{ 64, 96, std::vector<uint8_t>(64 * 96), { } };
                    for (int row = 0; row < 96; ++row)
                    {
                        for (int col = 0; col < 64; ++col)
                        {
                            int dx = left + col, dy = top + row;
                            if (dx >= 0 && dy >= 0 && dx < int(width * 64) && dy < int(height * 32))
                            {
                                background.pixels[size_t(row) * 64 + col] = pixels_[size_t(dy) * width * 64 + dx];
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
            mask_unit_ground(image.pixels, image.w, image.h, x, y, elevation);
            blit(image, x * 64 + (y & 1) * 32, y * 16 - (elevation - 1) * 16 - 64, true);
        }
        camera_x = 32;
        camera_y = 16;
        return Result<bool>::success(true);
    }
    Result<std::pair<int, int>> try_screen_position(int x, int y) const
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
    std::pair<int, int> screen_position(int x, int y) const { return try_screen_position(x, y).value(); }
    bool dialogue_visible(int x, int y, unsigned mode = 2) const
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
    std::pair<int, int> dialogue_camera(int x, int y) const
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
        return { std::max(1, half) * 32, std::max(1, row) * 16 };
    }
    void movement_camera(int x, int y)
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
    }
    Result<unsigned> try_idle_frame(unsigned appearance, uint64_t elapsed_ms = 0, unsigned direction = 0, unsigned action = 0) const
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
    unsigned idle_frame(unsigned appearance, uint64_t elapsed_ms = 0, unsigned direction = 0, unsigned action = 0) const { return try_idle_frame(appearance, elapsed_ms, direction, action).value(); }
    unsigned unit_height(unsigned appearance, unsigned direction = 0, unsigned action = 0) const
    {
        size_t index = group(1), frames = group(2);
        unsigned frame = idle_frame(appearance, 0, direction, action);
        unsigned first = word(prg_, index + 2 + (appearance - 1) * 8 + 2);
        size_t p = frames + dword(prg_, frames + 2 + (first + frame - 1) * 7);
        return word(prg_, p + 2) + 1;
    }
    // Renderer identities are indices in the full roster, never resource/story Unit::id values.
    std::vector<uint8_t> expected_reaction_bitmap(unsigned id, bool left) const
    {
        const auto& bitmap = unit_bitmaps_.at(id);
        return compose_reaction(bitmap.w, bitmap.h, bitmap.ad, bitmap.b1, left);
    }
    const std::vector<uint8_t>& unit_background_bitmap(unsigned id) const { return unit_bitmaps_.at(id).b1; }
    const std::vector<uint8_t>& unit_source_bitmap(unsigned id) const { return unit_bitmaps_.at(id).ad; }
    std::array<int, 4> unit_bitmap_bounds(unsigned id) const
    {
        const auto& b = unit_bitmaps_.at(id);
        return { b.left, b.top, int(b.w), int(b.h) };
    }
    bool begin_teleport_arrival(const std::vector<OriginalEvents::Unit>& roster, unsigned target, unsigned turn = 1, bool departure = false)
    {
        if (arrival_target_ || target >= roster.size())
        {
            return false;
        }
        auto image = draw();
        draw_units(image, roster, turn);
        if (!unit_bitmaps_.contains(target))
        {
            return false;
        }
        arrival_target_ = target;
        arrival_departure_ = departure;
        arrival_color_ = departure ? 254 : 194;
        arrival_frame_ = arrival_elapsed_ = 0;
        arrival_presented_ = false;
        return true;
    }
    bool teleport_arrival_active() const { return arrival_target_.has_value(); }
    unsigned teleport_arrival_frame() const { return arrival_frame_; }
    void teleport_arrival_presented() { arrival_presented_ = true; }
    void tick_teleport_arrival(unsigned ms)
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
    void cancel_teleport_arrival() { arrival_target_.reset(); }
    bool begin_disappearance(const std::vector<OriginalEvents::Unit>& roster, unsigned target, unsigned turn = 1, uint64_t elapsed = 0, bool death = false)
    {
        if (disappearance_.active())
        {
            return false;
        }
        auto role = target < roster.size() ? roster.begin() + target : roster.end();
        if (role == roster.end() || !role->present(turn) || !(role->status_flags & 0x80) || (role->status_flags & 0x20) || !role->appearance || role->appearance > word(prg_, group(1)))
        {
            return false;
        }
        // Initialize full indexed AD/B1; do not capture an RGBA framebuffer.
        auto scratch = draw();
        draw_units(scratch, roster, turn, elapsed);
        auto& bitmap = unit_bitmaps_.at(target);
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
        OriginalPalette palette(pal_);
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
    void enable_disappearance_presentation() { disappearance_.enable_presentation_timing(); }
    void disappearance_presented() { disappearance_.presented(); }
    void tick_disappearance(unsigned ms) { disappearance_.tick(ms); }
    bool disappearance_active() const { return disappearance_.active(); }
    bool disappearance_finished() const { return disappearance_.finished(); }
    unsigned disappearance_frame() const { return disappearance_.frame; }
    const std::vector<uint8_t>& disappearance_pixels() const { return disappearance_frames_.at(std::min(disappearance_.frame, disappearance_.frame_count - 1)); }
    unsigned last_reaction_target() const { return last_reaction_target_; }
    bool last_reaction_left() const { return last_reaction_left_; }
    uint64_t reaction_draws() const { return reaction_draws_; }
    const std::vector<uint8_t>& last_reaction_bitmap() const { return last_reaction_; }
    // Full roster order is required for 4485DB's persistent B1 overlays.
    Result<bool> draw_units_validated(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, unsigned turn = 1, uint64_t elapsed_ms = 0, std::optional<unsigned> silhouette = std::nullopt, std::optional<unsigned> reaction = std::nullopt, unsigned background_mode = 0, std::optional<unsigned> only_instance = std::nullopt, std::vector<uint8_t>* indexed = nullptr) const
    {
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
            auto frame_result = try_idle_frame(unit.appearance, unit.movement_pose ? uint64_t(unit.movement_frame) * 190 : idle_pose_time_.value_or(elapsed_ms), unit.direction, unit.animation_action);
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
            // 4409A4 stores table+6 at runtime+ACh; 4462B6 sign-extends it.
            int displacement = int(int8_t(prg_.at(table + 6)));
            int left = unit.x * 64 + (unit.y & 1) * 32 + 32 - int(w / 2) + displacement - camera_x;
            size_t map = item(6, map_id - 1), cell = map + 28 + (size_t(unit.y) * width + unit.x) * 5;
            int elevation = prg_.at(cell);
            mask_unit_ground(decoded, w, h, unit.x, unit.y, elevation, checker_mask_, displacement);
            // sub_42B1D2 anchors the foot at tile bottom minus (height-1)*16.
            int top = unit.y * 16 + 32 - int(h) - camera_y - std::max(0, elevation - 1) * 16;
            presented.push_back({ instance, unit, w, h, left, top, std::move(decoded) });
        }
        if (!silhouette)
        {
            // 4483CB mode0: crop the indexed world, then copy OTHER B1
            // buffers (including transparent bytes) in original roster order.
            for (auto id : changed)
            {
                auto& target = unit_bitmaps_.at(id);
                for (unsigned y = 0; y < target.h; ++y)
                {
                    for (unsigned x = 0; x < target.w; ++x)
                    {
                        int sx = target.left + int(x), sy = target.top + int(y);
                        target.b1[size_t(y) * target.w + x] = sx >= 0 && sy >= 0 && sx < int(width * 64) && sy < int(height * 32) ? pixels_.at(size_t(sy) * width * 64 + unsigned(sx)) : 0;
                    }
                }
                if (background_mode == 1)
                {
                    continue;
                }
                for (const auto& other : units)
                {
                    if (!other.present(turn) || (other.status_flags & 0x20) || !(other.status_flags & 0x80) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5)
                    {
                        continue;
                    }
                    auto found = unit_bitmaps_.find(unsigned(&other - units.data()));
                    if (found == unit_bitmaps_.end())
                    {
                        continue;
                    }
                    const auto& source = found->second;
                    if ((source.x == target.x && source.y == target.y) || std::abs(source.x - target.x) > 2 || std::abs(source.y - target.y) > 24)
                    {
                        continue;
                    }
                    overlay_bitmap(target, source, true);
                }
                if (background_mode == 2)
                {
                    for (const auto& other : units)
                    {
                        if (!other.present(turn) || (other.status_flags & 0x20) || !(other.status_flags & 0x80) || other.action_state == 1 || other.action_state == 3 || other.action_state == 5)
                        {
                            continue;
                        }
                        auto found = unit_bitmaps_.find(unsigned(&other - units.data()));
                        if (found == unit_bitmaps_.end())
                        {
                            continue;
                        }
                        const auto& source = found->second;
                        if ((source.x == target.x && source.y == target.y) || std::abs(source.x - target.x) > 2 || source.y >= target.y || source.y < target.y - 24)
                        {
                            continue;
                        }
                        overlay_bitmap(target, source, false);
                    }
                }
            }
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
                    movement_indices[size_t(y) * 640 + x] = indexed ? indexed->at(size_t(y) * 640 + x) : pixels_.at(size_t(camera_y + y) * width * 64 + camera_x + x);
                }
            }
        }
        auto staged_effect_clock = movement_effect_clock_;
        auto staged_effect_stamps = movement_effect_stamps_;
        for (auto& pose : presented)
        {
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
                auto shadow = original_movement::try_shadow(background, reaction_foot_mask_, reaction_shadow_);
                if (!shadow) { return Result<bool>::failure(shadow.error()); }
                for (const auto& other : presented)
                {
                    if (other.unit.action_state == 1 || other.unit.action_state == 3 || other.unit.action_state == 5 || std::abs(other.unit.x - pose.unit.x) > 2 || other.unit.y <= pose.unit.y || other.unit.y > pose.unit.y + 24) { continue; }
                    // 4468DF consumes raw AD, not the ground-masked display copy.
                    const auto& source = unit_bitmaps_.at(other.instance);
                    const int source_elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(other.unit.y) * width + other.unit.x) * 5);
                    auto erased = original_movement::try_erase(shadow.value(), 64, 32, source.ad, source.w, source.h,
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
            if (arrival_target_ && pose.instance == *arrival_target_ && !silhouette)
            {
                OriginalPalette palette(pal_);
                for (auto& ink : pose.pixels)
                {
                    if (ink)
                    {
                        ink = palette.blend_index(ink, arrival_color_, arrival_departure_ ? 10 + int(arrival_frame_) * 10 : 100 - int(arrival_frame_) * 10);
                    }
                }
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
            for (unsigned y = 0; y < pose.h; ++y)
            {
                for (unsigned x = 0; x < pose.w; ++x)
                {
                    unsigned c = pose.pixels.at(size_t(y) * pose.w + x);
                    int dx = pose.left + int(x), dy = pose.top + int(y);
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
                    for (unsigned k = 0; k < 3; ++k)
                    {
                        image.at(dst + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
                    }
                }
            }
        }
        return Result<bool>::success(true);
    }
    Result<bool> try_draw_units(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, unsigned turn = 1, uint64_t elapsed_ms = 0, std::optional<unsigned> silhouette = std::nullopt, std::optional<unsigned> reaction = std::nullopt, unsigned background_mode = 0, std::optional<unsigned> only_instance = std::nullopt, std::vector<uint8_t>* indexed = nullptr) const
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
        auto indices = indexed ? *indexed : std::vector<uint8_t>{ };
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
    void draw_units(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, unsigned turn = 1, uint64_t elapsed_ms = 0, std::optional<unsigned> silhouette = std::nullopt, std::optional<unsigned> reaction = std::nullopt, unsigned background_mode = 0, std::optional<unsigned> only_instance = std::nullopt, std::vector<uint8_t>* indexed = nullptr) const
    {
        auto result = try_draw_units(image, units, turn, elapsed_ms, silhouette, reaction, background_mode, only_instance, indexed);
        if (!result)
        {
            report_error(result.error());
        }
    }
    std::vector<uint8_t> skill32_background(const std::vector<OriginalEvents::Unit>& units, unsigned turn, unsigned caster, unsigned target, unsigned tint_source = 140, unsigned tint_amount = 45) const
    {
        auto image = draw();
        std::vector<uint8_t> indexed(640 * 480);
        for (unsigned y = 0; y < 480; ++y)
        {
            for (unsigned x = 0; x < 640; ++x)
            {
                indexed[size_t(y) * 640 + x] = pixels_.at(size_t(camera_y + y) * width * 64 + camera_x + x);
            }
        }
        draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, std::nullopt, &indexed);
        OriginalPalette palette(pal_);
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
        draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, caster);
        draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, target);
        return image;
    }
    std::vector<uint8_t> skill32_frame(const std::vector<uint8_t>& background, const OriginalSkill32Visual::Frame& frame, const std::vector<OriginalEvents::Unit>& units, unsigned turn, unsigned target, const OriginalSkill32Visual* visual = nullptr) const
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
        OriginalPalette palette(pal_);
        top -= camera_y + std::max(0, elevation - 1) * 16;
        left -= camera_x;
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
                    c = uint8_t(palette.blend_index(destinationColor, c, visual->amount(c)));
                }
                for (unsigned k = 0; k < 3; ++k)
                {
                    image.at((size_t(dy) * 640 + dx) * 4 + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
                }
            }
        }
        return image;
    }
    std::vector<uint8_t> skill88_background(const std::vector<OriginalEvents::Unit>& units, unsigned turn, unsigned caster, unsigned tint_amount = 50, unsigned tint_source = 233) const
    {
        auto image = draw();
        std::vector<uint8_t> indexed(640 * 480);
        for (unsigned y = 0; y < 480; ++y)
        {
            for (unsigned x = 0; x < 640; ++x)
            {
                indexed[size_t(y) * 640 + x] = pixels_.at(size_t(camera_y + y) * width * 64 + camera_x + x);
            }
        }
        draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, std::nullopt, &indexed);
        OriginalPalette palette(pal_);
        for (size_t p = 0; p < indexed.size(); ++p)
        {
            unsigned c = indexed[p];
            if (c)
            {
                c = palette.blend_index(c, tint_source, tint_amount);
            }
            indexed[p] = uint8_t(c);
            for (unsigned k = 0; k < 3; ++k)
            {
                image[p * 4 + k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
            }
        }
        draw_units(image, units, turn, 0, std::nullopt, std::nullopt, 0, caster, &indexed);
        skill88_indexed_ = std::move(indexed);
        return image;
    }
    std::vector<uint8_t> skill88_frame(const std::vector<uint8_t>& background, const OriginalSkill88Visual& visual, const OriginalEvents::Unit& actor) const
    {
        auto image = background;
        const auto& frame = visual.current();
        OriginalPalette palette(pal_);
        int left = actor.x * 64 + (actor.y & 1) * 32 - camera_x + 32 - visual.anchor_x(), top = actor.y * 16 - camera_y + 32 - visual.anchor_y();
        int elevation = prg_.at(item(6, map_id - 1) + 28 + (size_t(actor.y) * width + actor.x) * 5);
        top -= std::max(0, elevation - 1) * 16;
        for (unsigned y = 0; y < frame.height; ++y)
        {
            for (unsigned x = 0; x < frame.width; ++x)
            {
                unsigned c = frame.pixels[size_t(y) * frame.width + x];
                int dx = left + int(x), dy = top + int(y);
                if (!c || dx < 0 || dy < 0 || dx >= 640 || dy >= 480)
                {
                    continue;
                }
                size_t p = (size_t(dy) * 640 + dx) * 4;
                if (visual.amount(c) >= 0)
                {
                    c = palette.blend_index(skill88_indexed_.at(p / 4), c, visual.amount(c));
                }
                for (unsigned k = 0; k < 3; ++k)
                {
                    image[p + k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
                }
            }
        }
        return image;
    }
    void pan(int dx, int dy)
    {
        camera_x = std::clamp(camera_x + dx, 32, std::max(32, int(width * 64) - 21 * 32));
        camera_y = std::clamp(camera_y + dy, 16, std::max(16, int(height * 32) - 480));
    }
    bool edge_scroll(float x, float y)
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
    bool evacuation_visible(int x, int y) const
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
    bool evacuation_camera_step(int x, int y)
    {
        int elevation = int(prg_.at(item(6, map_id - 1) + 28 + (size_t(y) * width + x) * 5));
        int hx = std::max(1, std::min(2 * x - ((y & 1) ? 8 : 9), int(width) * 2 - 21));
        int vy = std::max(1, std::min(y - (elevation - 1 + 16), int(height) * 2 - 30));
        int cx = camera_x / 32, cy = camera_y / 16;
        cx += (cx < hx) - (cx > hx);
        cy += (cy < vy) - (cy > vy);
        camera_x = cx * 32;
        camera_y = cy * 16;
        return cx == hx && cy == vy;
    }
    void center(int x, int y)
    {
        camera_x = std::clamp(x * 64 + (y & 1) * 32 - 320, 0, int(width * 64) - 640);
        camera_y = std::clamp(y * 16 - 240, 0, int(height * 32) - 480);
    }
    std::vector<uint8_t> draw() const
    {
        std::vector<uint8_t> image(640 * 480 * 4);
        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                unsigned c = pixels_.at(size_t(camera_y + y) * width * 64 + camera_x + x);
                size_t dst = (size_t(y) * 640 + x) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    image[dst + k] = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
                }
                image[dst + 3] = 255;
            }
        }
        return image;
    }
    // 42FE12 supplies random object-name entries to 46243A and explicitly
    // skips type12; it is not a water-animation timer. Palette cycling is
    // applied by the presentation layer, not by changing map object IDs.
    std::vector<uint8_t> draw(uint64_t elapsed_ms) const
    {
        (void)elapsed_ms;
        return draw();
    }
};
}    //namespace s2ne
