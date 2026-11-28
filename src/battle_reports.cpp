#include "battle_reports.h"

namespace s2ne
{
bool BattleReports::detail_button_available(unsigned button) const
{
    return button < 2 || inspection_available_.at(size_t(first_ + row_)).at(button - 2);
}

s2ne::Result<SharedBytes> BattleReports::try_read(const std::filesystem::path& p)
{ return read_shared_resource(p); }

std::vector<uint8_t> BattleReports::bytes(const std::string& s)
{ return { s.begin(), s.end() }; }

void BattleReports::pixel(std::vector<uint8_t>& image, int x, int y, unsigned c) const
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480)
    {
        return;
    }
    size_t p = (size_t(y) * 640 + x) * 4;
    for (unsigned k = 0; k < 3; ++k)
    {
        image.at(p + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
    }
    image.at(p + 3) = 255;
}

s2ne::Result<bool> BattleReports::try_sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, bool transparent) const
{
    return draw_original_cel(image, cel_, pal_, id, left, top, transparent ? ZeroRun::transparent : ZeroRun::opaque);
}

void BattleReports::sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, bool transparent) const
{
    auto result = try_sprite(image, id, left, top, transparent);
    if (!result)
    {
        report_error(result.error());
    }
}

s2ne::Result<bool> BattleReports::large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color) const
{ return font_.try_draw_original_large(image, text, x, y, color); }

s2ne::Result<bool> BattleReports::small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color) const
{ return font_.try_draw_small(image, text, x, y, color, true); }

s2ne::Result<bool> BattleReports::try_detail(std::vector<uint8_t>& image, FullWidthCanvas* canvas, uint64_t elapsed) const
{
    ResourceReader palette_reader(pal_), archive(cel_);
    const size_t group = palette_reader.number(816, 4) - 57;
    auto dimmed = palette_.try_transform_rect(image, 0, 0, 640, 480, 5, canvas);
    if (!dimmed) { return dimmed; }
    auto panel = try_sprite(image, 0xDB, 0, 0, false);
    if (!panel) { return panel; }
    const auto& unit = selected().unit;
    auto status = status_.try_draw_report(font_, image, unit, 0, 0, selected().enemy);
    if (!status) { return status; }
    auto figure = unit_resources_.try_draw_status_figure(image, unit, 0x7D, 0x14A);
    if (!figure) { return figure; }
    const std::array<unsigned, 3> titles{ 0x1CC / 4, 4, 0x1C8 / 4 };
    const std::array<int, 3> values{ combat_values_.at(size_t(first_ + row_)), int(unit.experience), int(unit.experience_threshold) };
    for (unsigned i = 0; i < 3; ++i)
    {
        if (i == 0 && selected().enemy && !enemy_strength_visible_) { continue; }
        const int y = 0x3F + int(i) * 0x16;
        auto label = small(image, font_.text_bytes(titles[i]), 0x216, y, 179);
        if (!label) { return label; }
        auto value = small(image, bytes(std::to_string(values[i])), 0x240, y, 60);
        if (!value) { return value; }
    }
    if (!selected().enemy && selected().runtime_kind)
    {
        auto label = small(image, bytes("NPC"), 0x246, 0x1E, 38);
        if (!label) { return label; }
    }
    else
    {
        auto label = small(image, font_.text_bytes(selected().enemy ? 20 : 19), 0x246, 0x1E, selected().enemy ? 60 : 40);
        if (!label) { return label; }
    }
    unsigned emblem = 0;
    if (unit.movement_type >= 1 && unit.movement_type <= 5) { emblem = 0xE0; }
    else if (unit.movement_type >= 6 && unit.movement_type <= 9) { emblem = 0xE1 + unsigned(unit.movement_type - 6); }
    if (emblem)
    {
        auto icon = try_sprite(image, emblem, 0x226, 0x19);
        if (!icon) { return icon; }
    }
    if (unit.movement_type == 10)
    {
        auto icon = try_sprite(image, 0xE4, 0x226, 0x19);
        if (!icon) { return icon; }
        const size_t offset = archive.number(group + 2 + (0xE4 - 142) * 8, 4);
        const int stored_width = int(archive.number(group + offset));
        if (!archive) { return s2ne::Result<bool>::failure(archive.error()); }
        icon = try_sprite(image, 0xE0, 0x226 - stored_width, 0x19);
        if (!icon) { return icon; }
    }
    const int button_left = 0x1B2 + int(detail_button_) * 50, button_right = 0x1D9 + int(detail_button_) * 50;
    if (page_ != Page::detail)
    {
        auto pressed = try_sprite(image, 0xDC + detail_button_, button_left, 0x9A);
        if (!pressed) { return pressed; }
    }
    else
    {
        for (int y = 0x9A; y <= 0xB8; ++y)
        {
            for (int x = button_left; x <= button_right; ++x)
            {
                if (x == button_left || x == button_right || y == 0x9A || y == 0xB8) { pixel(image, x, y, 60); }
            }
        }
    }
    ResourceReader reader(items_);
    const unsigned count = reader.number(0);
    for (unsigned i = 0; i < unit.equipment.size(); ++i)
    {
        const unsigned id = unit.equipment[i];
        if (!id) { continue; }
        const size_t start = 2 + size_t(id - 1) * 38 + 2;
        if (id > count || !reader.extent(start, 12))
        {
            return s2ne::Result<bool>::failure(make_error("Invalid 456EEA equipment name"));
        }
        size_t end = start;
        while (end < start + 12 && items_[end]) { ++end; }
        auto name = large(image, { items_.begin() + start, items_.begin() + end }, 0x120, 0xA6 + int(i) * 0x23, 0xE9);
        if (!name) { return name; }
        auto icon = draw_original_cel(image, cel_, pal_, id + 0x1EC, 0xFE, 0xA1 + int(i) * 0x23, ZeroRun::transparent);
        if (!icon) { return icon; }
    }
    palette_.cycle_rect(image, 0xFE, 0xA1, 0x40, 0x23 * int(unit.equipment.size()), elapsed);
    return skills_.try_draw_status_effects(image, unit);
}

s2ne::Result<bool> BattleReports::try_roster_row_validated(std::vector<uint8_t>& image, int row, const Entry& entry) const
{
    int base = 5 + row * 118;
    auto panel = try_sprite(image, 206, 5, base);
    if (!panel)
    {
        return panel;
    }
    // 45706D draws the portrait and the row fields directly.  The old
    // implementation copied the battle status bar into the row, which is
    // a different caller (4571EB) and produced the visibly corrupt list.
    if (entry.unit.portrait)
    {
        size_t g = dword(pal_, 824) - 58, p = g + dword(cel_, g + 2 + (entry.unit.portrait * 2 - 1) * 8);
        auto portrait = draw_original_sprite(image, cel_, pal_, p, true, 12, base + 4, ZeroRun::transparent);
        if (!portrait)
        {
            return portrait;
        }
    }
    if (!(entry.unit.status_flags & 0x10))
    {
        auto name = large(image, entry.unit.name, 5 + 0x74, base + 7, entry.enemy ? 60 : 167);
        if (!name) { return name; }
    }
    auto title = entry.unit.profession;
    auto level = bytes(" " + std::to_string(entry.unit.level));
    title.insert(title.end(), level.begin(), level.end());
    auto suffix = font_.text_bytes(113);
    title.insert(title.end(), suffix.begin(), suffix.end());
    auto heading = large(image, title, 507 - int(title.size()) * 12, base + 7, 179);
    if (!heading) { return heading; }
    auto figure = unit_resources_.try_draw_status_figure(image, entry.unit, 5 + 0x230, base + 0x76);
    if (!figure) { return figure; }
    return status_.try_draw_status_chain(font_, image, entry.unit, 5 + 0x6E, base + 0x21);
}

s2ne::Result<bool> BattleReports::try_roster_row(std::vector<uint8_t>& image, int row, const Entry& entry) const
{
    auto output = image;
    auto result = try_roster_row_validated(output, row, entry);
    if (result)
    {
        image.swap(output);
    }
    return result;
}

void BattleReports::roster_row(std::vector<uint8_t>& image, int row, const Entry& entry) const
{
    auto result = try_roster_row(image, row, entry);
    if (!result)
    {
        report_error(result.error());
    }
}

s2ne::Result<bool> BattleReports::try_build_mini(Map map)
{
    int width = (int(map.width) * 64 - 64) / 3, height = (int(map.height) * 32 - 32) / 3;
    if (width <= 0 || height <= 0 || width > 640 || height > 480)
    {
        return s2ne::Result<bool>::failure(make_error("Original inspection map exceeds screen"));
    }
    mini_w_ = width;
    mini_h_ = height;
    mini_.resize(size_t(mini_w_) * mini_h_ * 4);
    int cached_x = -1, cached_y = -1;
    std::vector<uint8_t> tile;
    for (int y = 0; y < mini_h_; ++y)
    {
        for (int x = 0; x < mini_w_; ++x)
        {
            int sx = 31 + x * 3, sy = 16 + y * 3;
            int cx = std::min((sx / 640) * 640, int(map.width) * 64 - 640), cy = std::min((sy / 480) * 480, int(map.height) * 32 - 480);
            if (cx != cached_x || cy != cached_y)
            {
                map.camera_x = cx;
                map.camera_y = cy;
                tile = map.draw();
                cached_x = cx;
                cached_y = cy;
            }
            size_t source = (size_t(sy - cy) * 640 + sx - cx) * 4, dest = (size_t(y) * mini_w_ + x) * 4;
            for (unsigned k = 0; k < 4; ++k)
            {
                mini_[dest + k] = tile[source + k];
            }
        }
    }

    return s2ne::Result<bool>::success(true);
}

BattleReports::BattleReports(Menu font, UnitStatus status, SharedBytes cel, SharedBytes pal, Page page, const Events& events,
    const std::vector<uint8_t>& background) : font_(std::move(font)),
                                              status_(std::move(status)),
                                              cel_(std::move(cel)),
                                              pal_(std::move(pal)),
                                              palette_(pal_),
                                              background_(background),
                                              stage_name_(events.stage_name),
                                              victory_(events.victory_objective),
                                              defeat_(events.defeat_objective),
                                              stage_(events.stage),
                                              page_(page)
{
}

s2ne::Result<bool> BattleReports::initialize(const std::filesystem::path& root, Page page, const Events& events,
    std::vector<Entry> entries, const std::vector<uint8_t>& background, const Map* map, bool small_font, int dword_4f7cd0)
{
    if (background.size() != 640 * 480 * 4)
    {
        return s2ne::Result<bool>::failure(make_error("Battle report requires640x480 RGBA"));
    }
    enemy_strength_visible_ = events.army_modes && ((*events.army_modes)[1] & 4);
    auto skills = Skills::create(root, small_font);
    if (!skills) { return s2ne::Result<bool>::failure(skills.error()); }
    skills_ = std::move(skills).value();
    auto figures = Map::create(root);
    if (!figures) { return s2ne::Result<bool>::failure(figures.error()); }
    unit_resources_ = std::move(figures).value();
    auto items = try_read(root / "ITEMSP.BIN");
    if (!items) { return s2ne::Result<bool>::failure(items.error()); }
    items_ = std::move(items).value();
    auto kinds = try_read(root / "KINDSP.BIN");
    if (!kinds) { return s2ne::Result<bool>::failure(kinds.error()); }
    ResourceReader reader(kinds.value());
    size_t cursor = reader.number(6, 4);
    const unsigned count = reader.number(10);
    if (!reader) { return s2ne::Result<bool>::failure(reader.error()); }
    if (!count || count > 33 || cursor >= kinds.value().size())
    {
        return s2ne::Result<bool>::failure(make_error("Invalid 427BD5 ability name table"));
    }
    ability_names_.clear();
    for (unsigned i = 0; i < count; ++i)
    {
        const size_t start = cursor;
        while (cursor < kinds.value().size() && kinds.value()[cursor]) { ++cursor; }
        if (cursor == kinds.value().size())
        {
            return s2ne::Result<bool>::failure(make_error("Unterminated 427BD5 ability name"));
        }
        ability_names_.emplace_back(kinds.value().begin() + start, kinds.value().begin() + cursor);
        ++cursor;
    }
    font_.small_font = small_font;
    for (auto& entry : entries)
    {
        unsigned flags = entry.unit.status_flags;
        if ((flags & 0x20) || !(flags & 0x80))
        {
            continue;
        }
        if (entry.enemy)
        {
            ++enemy_count_;
        }
        else if (entry.runtime_kind == 1)
        {
            ++npc_count_;
        }
        else
        {
            ++friendly_count_;
        }
        auto value = events.try_combat_value(entry.unit);
        if (!value) { return s2ne::Result<bool>::failure(value.error()); }
        combat_values_.push_back(value.value());
        inspection_available_.push_back({ skills_.inspection_available(entry.unit, false), skills_.inspection_available(entry.unit, true) });
        entries_.push_back(std::move(entry));
    }
    if (page == Page::roster && entries_.empty())
    {
        page_ = Page::closed;
    }
    if (page == Page::conditions && dword_4f7cd0 == 1)
    {
        page_ = Page::closed;
    }
    if (page == Page::map)
    {
        if (!map)
        {
            return s2ne::Result<bool>::failure(make_error("Map inspection requires OriginalMap"));
        }
        auto mini = try_build_mini(*map);
        if (!mini)
        {
            return mini;
        }
    }

    return s2ne::Result<bool>::success(true);
}

BattleReports::BattleReports(const std::filesystem::path& root, Page page, const Events& events,
    std::vector<Entry> entries, const std::vector<uint8_t>& background, const Map* map, bool small_font, int dword_4f7cd0) : BattleReports(std::move(*create(root, page, events, std::move(entries), background, map, small_font, dword_4f7cd0).value()))
{
}

s2ne::Result<std::unique_ptr<BattleReports>> BattleReports::create(const std::filesystem::path& root, Page page, const Events& events,
    std::vector<Entry> entries, const std::vector<uint8_t>& background, const Map* map, bool small_font, int dword_4f7cd0)
{
    auto font = Menu::create(root);
    if (!font) { return s2ne::Result<std::unique_ptr<BattleReports>>::failure(font.error()); }
    auto status = UnitStatus::create(root);
    if (!status) { return s2ne::Result<std::unique_ptr<BattleReports>>::failure(status.error()); }
    auto cel = try_read(root / "SUPERSP.CEL");
    if (!cel) { return s2ne::Result<std::unique_ptr<BattleReports>>::failure(cel.error()); }
    auto pal = try_read(root / "SUPERSP.PAL");
    if (!pal) { return s2ne::Result<std::unique_ptr<BattleReports>>::failure(pal.error()); }
    if (pal.value().size() < 828) { return s2ne::Result<std::unique_ptr<BattleReports>>::failure(make_error("Truncated report palette")); }
    pal.value().mutable_at(0) = pal.value().mutable_at(1) = pal.value().mutable_at(2) = 0;
    auto result = std::unique_ptr<BattleReports>(new BattleReports(std::move(font).value(), std::move(*status.value()), std::move(cel).value(), std::move(pal).value(), page, events, background));
    auto ready = result->initialize(root, page, events, std::move(entries), background, map, small_font, dword_4f7cd0);
    if (!ready)
    {
        return s2ne::Result<std::unique_ptr<BattleReports>>::failure(ready.error());
    }
    return s2ne::Result<std::unique_ptr<BattleReports>>::success(std::move(result));
}

BattleReports::Page BattleReports::page() const
{ return page_; }

int BattleReports::first() const
{ return first_; }

int BattleReports::selected_row() const
{ return row_; }

size_t BattleReports::size() const
{ return entries_.size(); }

int BattleReports::return_code() const
{ return code_; }

std::optional<std::pair<int, int>> BattleReports::location() const
{ return locate_; }

const BattleReports::Entry& BattleReports::selected() const
{ return entries_.at(size_t(first_ + row_)); }

unsigned BattleReports::detail_address() const
{ return detail_address_; }

void BattleReports::close_to_battle(bool enabled)
{ close_to_battle_ = enabled; }

void BattleReports::close_detail()
{
    if (page_ == Page::abilities || page_ == Page::proficiencies || page_ == Page::status || page_ == Page::equipment)
    {
        page_ = Page::detail;
        detail_address_ = 0x455C7B;
    }
    else if (page_ == Page::detail)
    {
        page_ = close_to_battle_ ? Page::closed : Page::roster;
        detail_address_ = 0;
    }
}

void BattleReports::proficiencies()
{
    if (page_ == Page::detail)
    {
        page_ = Page::proficiencies;
        detail_address_ = 0x456BC0;
    }
}

void BattleReports::abilities()
{
    if (page_ == Page::detail)
    {
        page_ = Page::abilities;
        detail_address_ = 0x4569E2;
    }
}

void BattleReports::status_page()
{
    if (page_ == Page::detail && detail_button_available(2))
    {
        page_ = Page::status;
        skills_.open_inspection(selected().unit, false);
        detail_address_ = 0x456FC4;
    }
}

void BattleReports::equipment_page()
{
    if (page_ == Page::detail && detail_button_available(3))
    {
        page_ = Page::equipment;
        skills_.open_inspection(selected().unit, true);
        detail_address_ = 0x456FC4;
    }
}

void BattleReports::detail()
{
    if (page_ == Page::roster)
    {
        detail_button_ = 0;
        detail_address_ = 0x455C7B;
        page_ = Page::detail;
    }
}

void BattleReports::locate()
{
    if (page_ == Page::roster && !selected().locate_blocked)
    {
        locate_ = { { selected().unit.x, selected().unit.y } };
        code_ = 1;
        page_ = Page::closed;
    }
}

void BattleReports::navigate(unsigned scan)
{
    if (page_ != Page::roster)
    {
        return;
    }
    int count = int(entries_.size());
    switch (scan)
    {
    case 0x48:
        if (row_ > 0)
        {
            --row_;
        }
        else if (first_ > 0)
        {
            --first_;
        }
        break;
    case 0x50:
        if (first_ + row_ < count - 1)
        {
            if (row_ < 3)
            {
                ++row_;
            }
            else
            {
                ++first_;
            }
        }
        break;
    case 0x49:
        if (first_ > 0)
        {
            first_ = std::max(0, first_ - 4);
        }
        else
        {
            row_ = 0;
        }
        break;
    case 0x51:
        if (first_ + row_ + 8 < count - 1)
        {
            first_ += 4;
        }
        else if (first_ + 4 <= count - 1)
        {
            first_ = count - 4;
        }
        else
        {
            row_ = count - first_ - 1;
        }
        break;
    case 0x47: first_ = row_ = 0; break;
    case 0x4F:
        if (count - 4 > first_)
        {
            first_ = count - 4;
            row_ = 0;
        }
        break;
    }
}

int BattleReports::hit(int x, int y) const
{
    if (x >= 600 || page_ != Page::roster)
    {
        return -1;
    }
    for (int i = 0; i < 4 && first_ + i < int(entries_.size()); ++i)
    {
        if (y >= 5 + i * 118 && y <= 122 + i * 118)
        {
            return i;
        }
    }
    return -1;
}

BattleReports::Result BattleReports::event(const Settings::Event& e)
{
    if (page_ == Page::closed)
    {
        return {};
    }
    bool key = e.type == 1 || e.type == 2, mouse = e.type == 8 || e.type == 0x20;
    if (page_ == Page::conditions || page_ == Page::map)
    {
        if ((key && e.scan) || (mouse && e.buttons))
        {
            page_ = Page::closed;
            return { true, 0x1F };
        }
        return {};
    }
    if (page_ == Page::abilities || page_ == Page::proficiencies || page_ == Page::status || page_ == Page::equipment)
    {
        if (page_ == Page::status || page_ == Page::equipment)
        {
            if (skills_.inspection_help())
            {
                if ((key && e.scan) || (mouse && e.buttons))
                {
                    skills_.close_inspection_help();
                    return { true, 0x1F };
                }
                return {};
            }
            if ((key && e.scan == 1) || (mouse && (e.buttons & 2)))
            {
                close_detail();
                return { true, 0x1F };
            }
            if (key) { skills_.navigate_inspection(e.scan, (e.modifiers & 0x20) != 0); }
            const bool help = (key && (e.scan == 0x39 || e.scan == 0x3B)) || (mouse && skills_.mouse_inspection(e.x, e.y, (e.buttons & 1) != 0));
            if (help)
            {
                auto opened = skills_.try_open_inspection_help();
                if (!opened) { report_error(opened.error()); }
            }
            return { key || mouse, 0x1F };
        }
        if ((key && e.scan) || (mouse && e.buttons))
        {
            close_detail();
            return { true, 0x1F };
        }
        return {};
    }
    if (page_ == Page::detail)
    {
        if (key && (e.scan == 0x4B || e.scan == 0x4D))
        {
            do { detail_button_ = (detail_button_ + (e.scan == 0x4B ? 3 : 1)) % 4; } while (!detail_button_available(detail_button_));
            return { true, 0x1F };
        }
        const bool over_button = mouse && e.x >= 0x1B2 && e.x < 0x27A && e.y >= 0x9A && e.y <= 0xB9;
        const unsigned hovered = over_button ? unsigned((e.x - 0x1B2) / 0x32) : detail_button_;
        if (over_button && detail_button_available(hovered)) { detail_button_ = hovered; }
        if ((over_button && hovered == detail_button_ && (e.buttons & 1)) || (key && (e.scan == 0x1C || e.scan == 0x39)))
        {
            switch (detail_button_)
            {
            case 0: abilities(); break;
            case 1: proficiencies(); break;
            case 2: status_page(); break;
            case 3: equipment_page(); break;
            default: break;
            }
            return { true, 0x1F };
        }
        if ((key && e.scan == 1) || (mouse && (e.buttons & 2)))
        {
            close_detail();
            return { true, 0x1F };
        }
        if (over_button) { return { true, 0x1F }; }
        return {};
    }
    if (key)
    {
        if (e.scan == 1)
        {
            page_ = Page::closed;
            return { true, 4 };
        }
        if (e.scan == 0x39)
        {
            detail();
            return { true, 4 };
        }
        if (e.scan == 0x1C)
        {
            locate();
            return { true, 4 };
        }
        navigate(e.scan);
        return { true, 0 };
    }
    if (mouse)
    {
        int row = hit(e.x, e.y);
        if (row >= 0)
        {
            row_ = row;
        }
        if (e.buttons & 1)
        {
            if (e.x >= 616 && e.x <= 635)
            {
                if (e.y >= 6 && e.y <= 29)
                {
                    navigate(0x48);
                }
                else if (e.y >= 452 && e.y <= 475)
                {
                    navigate(0x50);
                }
                else if (e.y >= 31 && e.y <= 450)
                {
                    int n = int(size());
                    first_ = std::max(0, std::min((e.y - 31) * n / 419, n - 4));
                    row_ = std::min(e.y >= 422 ? 3 : 0, n - first_ - 1);
                }
            }
            else if (row >= 0 && e.y >= 9 + row * 118 && e.y <= 119 + row * 118)
            {
                if (e.x >= 12 && e.x <= 104)
                {
                    detail();
                }
                else if (e.x >= 520 && e.x < 600)
                {
                    locate();
                }
            }
            return { true, 0x10 };
        }
        if (e.buttons & 2)
        {
            page_ = Page::closed;
            return { true, 0x10 };
        }
        return { row >= 0, 0 };
    }
    return {};
}

s2ne::Result<std::vector<uint8_t>> BattleReports::try_draw(FullWidthCanvas* canvas) const
{
    const auto elapsed = texture_resources.elapsed();
    auto image = background_;
    if (page_ == Page::closed)
    {
        return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
    }
    if (page_ == Page::map)
    {
        int left = 320 - mini_w_ / 2, top = 240 - mini_h_ / 2;
        for (int y = 0; y < mini_h_; ++y)
        {
            for (int x = 0; x < mini_w_; ++x)
            {
                for (unsigned k = 0; k < 4; ++k)
                {
                    image[(size_t(top + y) * 640 + left + x) * 4 + k] = mini_[(size_t(y) * mini_w_ + x) * 4 + k];
                }
            }
        }
        return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
    }
    if (page_ == Page::conditions)
    {
        int left = 10, top = 52;
        {
            auto rendered = try_sprite(image, 207, left, top);
            if (!rendered)
            {
                return s2ne::Result<std::vector<uint8_t>>::failure(rendered.error());
            }
        }
        auto heading = large(image, font_.text_bytes(47), left + 320 - int(font_.text_bytes(47).size()) * 6, top + 17, 57);
        if (!heading) { return s2ne::Result<std::vector<uint8_t>>::failure(heading.error()); }
        auto stage_label = large(image, font_.text_bytes(48), left + 90, top + 57, 54);
        if (!stage_label) { return s2ne::Result<std::vector<uint8_t>>::failure(stage_label.error()); }
        auto title = bytes(std::to_string(stage_) + ".");
        title.insert(title.end(), stage_name_.begin(), stage_name_.end());
        auto name = large(image, title, left + 213, top + 57, 54);
        if (!name) { return s2ne::Result<std::vector<uint8_t>>::failure(name.error()); }
        auto victory_label = large(image, font_.text_bytes(49), left + 90, top + 108, 119);
        if (!victory_label) { return s2ne::Result<std::vector<uint8_t>>::failure(victory_label.error()); }
        auto defeat_label = large(image, font_.text_bytes(50), left + 90, top + 208, 87);
        if (!defeat_label) { return s2ne::Result<std::vector<uint8_t>>::failure(defeat_label.error()); }
        auto victory = font_.try_draw_original_wrapped(image, victory_, left + 213, top + 108, 119, 4, 11, 28, 42);
        if (!victory) { return s2ne::Result<std::vector<uint8_t>>::failure(victory.error()); }
        auto defeat = font_.try_draw_original_wrapped(image, defeat_, left + 213, top + 208, 87, 4, 11, 28, 42);
        if (!defeat) { return s2ne::Result<std::vector<uint8_t>>::failure(defeat.error()); }
        const std::array<int, 3> xs{ 86, 264, 436 }, numbers{ friendly_count_, npc_count_, enemy_count_ };
        auto friendly = large(image, font_.text_bytes(19), left + xs[0], top + 330, 40);
        if (!friendly) { return s2ne::Result<std::vector<uint8_t>>::failure(friendly.error()); }
        auto npc = large(image, bytes("NPC"), left + xs[1], top + 330, 38);
        if (!npc) { return s2ne::Result<std::vector<uint8_t>>::failure(npc.error()); }
        auto enemy = large(image, font_.text_bytes(20), left + xs[2], top + 330, 60);
        if (!enemy) { return s2ne::Result<std::vector<uint8_t>>::failure(enemy.error()); }
        for (int i = 0; i < 3; ++i)
        {
            auto number = large(image, bytes(std::to_string(numbers[i])), left + 144 + 176 * i, top + 330, 185);
            if (!number) { return s2ne::Result<std::vector<uint8_t>>::failure(number.error()); }
        }
        return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
    }
    if (page_ == Page::abilities || page_ == Page::proficiencies)
    {
        auto status = try_detail(image, canvas, elapsed);
        if (!status) { return s2ne::Result<std::vector<uint8_t>>::failure(status.error()); }
    }
    if (page_ == Page::proficiencies)
    {
        ResourceReader palette(pal_), archive(cel_);
        const unsigned address = palette.number(816, 4);
        if (!palette) { return s2ne::Result<std::vector<uint8_t>>::failure(palette.error()); }
        if (address < 57) { return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Invalid proficiency CEL directory")); }
        const size_t group = address - 57;
        const size_t offset = archive.number(group + 2 + (0xE8 - 142) * 8, 4);
        const int top = 480 - int(archive.number(group + offset + 2)) - 1;
        if (!archive) { return s2ne::Result<std::vector<uint8_t>>::failure(archive.error()); }
        auto panel = try_sprite(image, 0xE8, 0, top, true);
        if (!panel) { return s2ne::Result<std::vector<uint8_t>>::failure(panel.error()); }
        const auto title = font_.text_bytes(0x27C / 4);
        auto heading = large(image, title, 320 - int(title.size()) * 6, top + 8, 0x49);
        if (!heading) { return s2ne::Result<std::vector<uint8_t>>::failure(heading.error()); }
        auto weapon = large(image, font_.text_bytes(0xA8 / 4), 0x87, top + 8, 0x49);
        if (!weapon) { return s2ne::Result<std::vector<uint8_t>>::failure(weapon.error()); }
        auto learned = large(image, font_.text_bytes(0x280 / 4), 0x1C6, top + 8, 0x49);
        if (!learned) { return s2ne::Result<std::vector<uint8_t>>::failure(learned.error()); }
        const auto& unit = selected().unit;
        for (unsigned i = 0; i < 6; ++i)
        {
            const int x = i < 5 ? 0x154 : 0x1EA;
            const int y = 0x181 + int(i % 5) * 16;
            auto label = small(image, font_.text_bytes(0x2B0 / 4 + i), x, y, 0xE9);
            if (!label) { return s2ne::Result<std::vector<uint8_t>>::failure(label.error()); }
            auto value = small(image, bytes(std::to_string(unit.weapon_proficiency[i])), x + 0x50, y, 0x89);
            if (!value) { return s2ne::Result<std::vector<uint8_t>>::failure(value.error()); }
        }
        for (unsigned i = 0; i < 9; ++i)
        {
            const int x = i < 5 ? 0x14 : 0xA5;
            const int y = 0x181 + int(i % 5) * 16;
            auto label = small(image, font_.text_bytes(0x284 / 4 + i), x, y, 0xE9);
            if (!label) { return s2ne::Result<std::vector<uint8_t>>::failure(label.error()); }
            auto value = small(image, bytes(std::to_string(unit.learned_proficiency[i] / 10)), x + 0x60, y, 0x89);
            if (!value) { return s2ne::Result<std::vector<uint8_t>>::failure(value.error()); }
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            const int y = 0x1B1 + int(i) * 16;
            auto label = small(image, font_.text_bytes(0x2A8 / 4 + i), 0x1EA, y, 0xE9);
            if (!label) { return s2ne::Result<std::vector<uint8_t>>::failure(label.error()); }
            auto value = small(image, bytes(std::to_string(unit.learned_proficiency[9 + i] / 10)), 0x23A, y, 0x89);
            if (!value) { return s2ne::Result<std::vector<uint8_t>>::failure(value.error()); }
        }
        return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
    }
    if (page_ == Page::abilities)
    {
        ResourceReader palette(pal_), archive(cel_);
        const unsigned address = palette.number(816, 4);
        if (!palette) { return s2ne::Result<std::vector<uint8_t>>::failure(palette.error()); }
        if (address < 57) { return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Invalid ability CEL directory")); }
        const size_t group = address - 57;
        const size_t offset = archive.number(group + 2 + (0xE6 - 142) * 8, 4);
        const int top = 480 - int(archive.number(group + offset + 2)) - 1;
        if (!archive) { return s2ne::Result<std::vector<uint8_t>>::failure(archive.error()); }
        auto panel = try_sprite(image, 0xE6, 0, top, true);
        if (!panel) { return s2ne::Result<std::vector<uint8_t>>::failure(panel.error()); }
        const auto title = font_.text_bytes(0x278 / 4);
        auto heading = large(image, title, 320 - int(title.size()) * 6, top + 7, 0x49);
        if (!heading) { return s2ne::Result<std::vector<uint8_t>>::failure(heading.error()); }
        const uint32_t flags = selected().unit.abilities;
        if (!flags)
        {
            const auto& none = ability_names_.at(0);
            auto label = large(image, none, 320 - int(none.size()) * 6, top + 0x3D, 0xE9);
            if (!label) { return s2ne::Result<std::vector<uint8_t>>::failure(label.error()); }
            return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
        }
        unsigned learned = 0;
        for (unsigned i = 1; i < ability_names_.size(); ++i)
        {
            if (flags & (1u << (i - 1))) { ++learned; }
        }
        int x = 324 - ((int(learned) - 1) / 2 + 1) * 0x24;
        int y = top + 0x34;
        unsigned drawn = 0;
        for (unsigned i = 1; i < ability_names_.size(); ++i)
        {
            if (!(flags & (1u << (i - 1)))) { continue; }
            auto name = large(image, ability_names_[i], x, y, 0xE9);
            if (!name) { return s2ne::Result<std::vector<uint8_t>>::failure(name.error()); }
            ++drawn;
            if (drawn & 1) { y += 0x1E; }
            else
            {
                x += 0x48;
                y = top + 0x34;
            }
        }
        return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
    }
    if (page_ == Page::status || page_ == Page::equipment)
    {
        auto detail = try_detail(image, canvas, elapsed);
        if (!detail) { return s2ne::Result<std::vector<uint8_t>>::failure(detail.error()); }
        auto menu = skills_.try_draw_inspection(image, selected().unit, canvas);
        if (!menu) { return s2ne::Result<std::vector<uint8_t>>::failure(menu.error()); }
        return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
    }
    if (page_ == Page::detail)
    {
        // 456179 starts from CEL 0xDB and clears a complete dedicated
        // detail page; it does not draw into the fourth roster row.
        auto status = try_detail(image, canvas, elapsed);
        if (!status)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(status.error());
        }
        return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
    }
    {
        auto rendered = try_sprite(image, 205, 0, 0, false);
        if (!rendered)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(rendered.error());
        }
    }
    for (int i = 0; i < 4 && first_ + i < int(size()); ++i)
    {
        {
            auto rendered = try_roster_row(image, i, entries_[first_ + i]);
            if (!rendered)
            {
                return s2ne::Result<std::vector<uint8_t>>::failure(rendered.error());
            }
        }
    }
    for (int y = 9 + row_ * 118; y <= 118 + row_ * 118; ++y)
    {
        for (int x = 12; x <= 104; ++x)
        {
            if (x == 12 || x == 104 || y == 9 + row_ * 118 || y == 118 + row_ * 118)
            {
                pixel(image, x, y, 60);
            }
        }
    }
    int thumb = 31;
    if (first_ + row_ > 0)
    {
        thumb += (first_ + row_ + 1) * 391 / int(size());
    }
    {
        auto rendered = try_sprite(image, 147, 616, thumb, true);
        if (!rendered)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(rendered.error());
        }
    }
    return s2ne::Result<std::vector<uint8_t>>::success(std::move(image));
}

std::vector<uint8_t> BattleReports::draw(FullWidthCanvas* canvas) const
{
    auto result = try_draw(canvas);
    return std::move(result).value();
}
}    //namespace s2ne
