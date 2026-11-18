#pragma once
#include "battle.hpp"
#include "original_army_equipment_menu.hpp"
#include "original_battle_reports.hpp"

namespace s2ne
{
// 原44AED2在出阵确认前只修改候选军团，不发布存档状态。
class OriginalDeployment
{
    std::filesystem::path root_;
    OriginalDisc disc_;
    OriginalMenu font_;
    OriginalShopMenu graphics_;
    OriginalMap map_;
    std::optional<Battle> picker_;
    OriginalEvents events_;
    OriginalSave::State raw_;
    std::vector<size_t> roles_;
    std::vector<OriginalEvents::DeploymentSlot> slots_;
    std::vector<int> assignment_;
    std::vector<bool> fixed_;
    std::vector<uint8_t> cells_, palette_;
    std::vector<uint8_t> placement_cell_;
    std::array<size_t, 2> backgrounds_{};
    size_t first_{}, row_{}, option_{}, placing_{};
    bool buttons_{}, position_{}, facing_{}, done_{}, inspect_{};
    unsigned notice_{};
    std::optional<OriginalConfirmation> confirmation_;
    std::optional<OriginalArmyEquipmentMenu> equipment_;
    std::optional<OriginalBattleReports> report_;
    std::optional<OriginalEvents::Unit> before_position_;
    int before_slot_{ -1 };
    Battle::Point cursor_{ -1, -1 };
    static unsigned word(const auto& b, size_t p) { return unsigned(b.at(p)) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    size_t selected() const { return roles_.at(first_ + row_); }
    size_t deployed_count() const
    {
        return size_t(std::count_if(assignment_.begin(), assignment_.end(), [](int role)
            {
                return role >= 0;
            }));
    }
    int slot_for(size_t role) const
    {
        auto found = std::find(assignment_.begin(), assignment_.end(), int(role));
        return found == assignment_.end() ? -1 : int(found - assignment_.begin());
    }
    void assign(size_t role, size_t slot)
    {
        assignment_.at(slot) = int(role);
        auto& u = events_.units.at(role);
        const auto& p = slots_.at(slot);
        u.x = p.x;
        u.y = p.y;
        u.direction = p.direction & 3;
        u.animation_action = p.direction >> 2;
        u.status_flags |= 0x80;
    }
    void cancel_position()
    {
        if (before_position_)
        {
            events_.units.at(placing_) = std::move(*before_position_);
            if (before_slot_ >= 0) { assignment_.at(size_t(before_slot_)) = int(placing_); }
        }
        before_position_.reset();
        confirmation_.reset();
        position_ = facing_ = inspect_ = false;
    }
    void begin_position(size_t role)
    {
        placing_ = role;
        before_position_ = events_.units.at(role);
        before_slot_ = slot_for(role);
        if (before_slot_ >= 0) { assignment_.at(size_t(before_slot_)) = -1; }
        events_.units.at(role).status_flags &= ~0x80u;
        position_ = true;
        if (before_slot_ >= 0) { map_.center(before_position_->x, before_position_->y); }
        else
        {
            for (size_t s = 0; s < assignment_.size(); ++s)
            {
                if (assignment_[s] >= 0) { continue; }
                map_.center(slots_[s].x, slots_[s].y);
                break;
            }
        }
        map_.pan(0, 0);
    }
    void toggle_role()
    {
        size_t role = selected();
        int slot = slot_for(role);
        if (slot < 0 && deployed_count() == capacity())
        {
            notice_ = 43;
            return;
        }
        if (slot >= 0 && !fixed_.at(role))
        {
            assignment_.at(size_t(slot)) = -1;
            events_.units.at(role).status_flags &= ~0x80u;
            return;
        }
        begin_position(role);
    }
    Result<bool> capture_equipment()
    {
        auto restored = OriginalSaveBridge::try_restore(root_, raw_);
        if (!restored) { return Result<bool>::failure(restored.error()); }
        auto next = std::move(restored).value().events;
        for (size_t i = 0; i < raw_.groups[0].units.size(); ++i)
        {
            auto& u = next.units.at(i);
            const auto& prior = events_.units.at(i);
            u.x = prior.x;
            u.y = prior.y;
            u.direction = prior.direction;
            u.animation_action = prior.animation_action;
            u.status_flags = prior.status_flags;
        }
        events_.units.swap(next.units);
        return Result<bool>::success(true);
    }
    void activate()
    {
        switch (option_)
        {
        case 0:
            for (size_t s = 0; s < capacity(); ++s)
            {
                if (assignment_[s] >= 0) { continue; }
                auto found = std::find_if(roles_.begin(), roles_.end(), [&](size_t role)
                    {
                        return slot_for(role) < 0 && int(events_.units[role].id) == slots_[s].role;
                    });
                if (found == roles_.end())
                {
                    found = std::find_if(roles_.begin(), roles_.end(), [&](size_t role)
                        {
                            return slot_for(role) < 0;
                        });
                }
                if (found == roles_.end()) { break; }
                assign(*found, s);
            }
            done_ = true;
            break;
        case 1:
            position_ = inspect_ = true;
            map_.center(events_.units[selected()].x, events_.units[selected()].y);
            break;
        case 2:
            if (deployed_count() <= 1) { confirmation_.emplace(root_, font_, 6, 280, 260, true); }
            else
            {
                done_ = true;
            }
            break;
        case 3: confirmation_.emplace(root_, font_, 6, 390, 260, false); break;
        case 4:
        {
            auto captured = OriginalSaveBridge::try_capture(events_, 1, { map_.camera_x, map_.camera_y }, &raw_);
            if (!captured)
            {
                report_error(captured.error());
                return;
            }
            raw_ = std::move(captured).value();
            auto menu = OriginalArmyEquipmentMenu::create(root_, raw_, false, disc_);
            if (!menu)
            {
                report_error(menu.error());
                return;
            }
            equipment_.emplace(std::move(menu).value());
            break;
        }
        }
    }
    void resolve_confirmation()
    {
        if (!confirmation_ || !confirmation_->result()) { return; }
        bool yes = *confirmation_->result();
        confirmation_.reset();
        if (facing_)
        {
            if (!yes)
            {
                cancel_position();
                return;
            }
            auto p = cursor_;
            for (size_t s = 0; s < capacity(); ++s)
            {
                if (assignment_[s] < 0 && slots_[s].x == p.x && slots_[s].y == p.y)
                {
                    unsigned direction = events_.units.at(placing_).direction;
                    assign(placing_, s);
                    events_.units.at(placing_).direction = direction;
                    break;
                }
            }
            before_position_.reset();
            position_ = facing_ = false;
            if (deployed_count() == capacity() || deployed_count() == roles_.size())
            {
                buttons_ = true;
                option_ = 2;
            }
        }
        else if (option_ == 2) { done_ = yes; }
        else if (option_ == 3 && yes)
        {
            auto captured = OriginalSaveBridge::try_capture(events_, 1, { map_.camera_x, map_.camera_y }, &raw_);
            if (!captured)
            {
                report_error(captured.error());
                return;
            }
            auto staged = std::move(captured).value();
            auto equipped = OriginalEquipment(root_).try_auto_equip(staged);
            if (!equipped)
            {
                report_error(equipped.error());
                return;
            }
            auto baseline = raw_;
            raw_ = std::move(staged);
            auto updated = capture_equipment();
            if (!updated)
            {
                raw_ = std::move(baseline);
                report_error(updated.error());
            }
        }
    }
    void inspect_role()
    {
        for (const auto& u : events_.units)
        {
            if (!(u.status_flags & 0x80) || (u.status_flags & 0x20) || u.x != cursor_.x || u.y != cursor_.y) { continue; }
            auto background = map_.draw();
            map_.draw_units(background, events_.units, 1, 0);
            report_.emplace(root_, OriginalBattleReports::Page::detail, events_, std::vector<OriginalBattleReports::Entry>{ { u, u.hostile() } }, background);
            report_->close_to_battle();
            break;
        }
    }
    void navigate(bool right)
    {
        if (buttons_)
        {
            option_ = (option_ + (right ? 1 : 4)) % 5;
            return;
        }
        if (right && first_ + row_ + 1 < roles_.size())
        {
            if (row_ < 4) { ++row_; }
            else
            {
                ++first_;
            }
        }
        else if (!right)
        {
            if (row_) { --row_; }
            else if (first_) { --first_; }
        }
    }
    void outline(std::vector<uint8_t>& image, int left, int top, int right, int bottom, unsigned color) const
    {
        for (int y = std::max(0, top); y <= std::min(479, bottom); ++y)
        {
            for (int x = std::max(0, left); x <= std::min(639, right); ++x)
            {
                if (x != left && x != right && y != top && y != bottom) { continue; }
                size_t p = (size_t(y) * 640 + x) * 4;
                for (unsigned c = 0; c < 3; ++c) { image[p + c] = uint8_t(unsigned(palette_[color * 3 + c]) * 255 / 63); }
                image[p + 3] = 255;
            }
        }
    }

public:
    OriginalDeployment(const std::filesystem::path& root, OriginalEvents events, OriginalSave::State raw, OriginalDisc disc) : root_(root), disc_(disc), font_(root), graphics_(root, disc), map_(root, disc), events_(std::move(events)), raw_(std::move(raw))
    {
        auto picker = Battle::try_load(root, events_.map_id, disc);
        if (!picker) { fatal(picker.error()); }
        picker_.emplace(std::move(picker).value());
        slots_ = events_.deployment_slots();
        fixed_.resize(events_.units.size());
        for (size_t i = 0; i < raw_.groups[0].units.size(); ++i)
        {
            auto& u = events_.units.at(i);
            // 4414FC(mode=1)：0x20、0x40、控制类型1和0x10均不列入出阵候选。
            const bool third_route_role = events_.stage >= 201 && (u.id == 11 || u.id == 14 || u.id == 15 || u.id == 13 || u.id == 1 || u.id == 2 || u.id == 3 || u.id == 4);
            if (!(u.status_flags & 0x70) && u.behavior != 1 && word(raw_.groups[0].units[i], 8) != 1 && (events_.stage < 201 || third_route_role))
            {
                roles_.push_back(i);
                // 原4414FC(mode=1)只重建可部署候选的登场状态。
                u.status_flags &= ~0x80u;
            }
        }
        if (roles_.empty() || slots_.empty())
        {
            if (events_.stage >= 201)
            {
                fatal("Third-route deployment requires both protagonists");
            }
            done_ = true;
            return;
        }
        if (events_.stage >= 201 && roles_.size() < 8)
        {
            fatal("Third-route deployment requires all eight core roster members");
        }
        assignment_.assign(capacity(), -1);
        if (events_.stage >= 201)
        {
            size_t mandatory_count = 0;
            for (size_t role : roles_)
            {
                const int id = int(events_.units[role].id);
                if (id == 11 || id == 1)
                {
                    if (mandatory_count >= capacity()) { fatal("Third-route deployment has no slot for protagonist"); }
                    assign(role, mandatory_count);
                    fixed_[role] = true;
                    ++mandatory_count;
                }
            }
        }
        // 原44AED2先保留指定角色；没有指定角色时仅默认第一人出阵。
        for (size_t s = 0; s < capacity(); ++s)
        {
            if (!slots_[s].fixed) { continue; }
            for (size_t role : roles_)
            {
                if (slot_for(role) < 0 && int(events_.units[role].id) == slots_[s].role)
                {
                    assign(role, s);
                    fixed_[role] = true;
                    break;
                }
            }
        }
        if (!deployed_count()) { assign(roles_.front(), 0); }
        if (capacity() == 1) { done_ = true; }
        palette_ = read_resource(root / "SUPERSP.PAL").value();
        cells_ = OriginalDiscResources(root).dat_entry(disc, 7);
        auto icons = OriginalDiscResources(root).dat_entry(disc, 5);
        size_t placement = dword(icons, 2 + 33 * 8);
        if (word(icons, placement) != 63 || word(icons, placement + 2) != 31) { fatal("Invalid original deployment placement cell"); }
        placement_cell_.assign(icons.begin() + placement + 4, icons.begin() + placement + 2052);
        for (size_t i = 0; i < 2; ++i) { backgrounds_[i] = dword(cells_, 2 + i * 8); }
        map_.load(events_.map_id);
        std::print(stderr, "Original deployment ctor map loaded stage={} slots={} roles={}\n", events_.stage, slots_.size(), roles_.size());
        picker_->set_mask_surfaces(true);
    }
    size_t capacity() const { return std::min(slots_.size(), roles_.size()); }
    bool done() const { return done_; }
    bool positioning() const { return position_; }
    bool facing() const { return facing_; }
    const auto& roles() const { return roles_; }
    const auto& assignments() const { return assignment_; }
    const auto& events() const { return events_; }
    const auto& raw() const { return raw_; }
    const auto& slots() const { return slots_; }
    auto cell_screen(int x, int y) const
    {
        auto result = map_.try_screen_position(x, y);
        if (!result)
        {
            const auto [width, height] = map_.dimensions();
            fatal("Invalid third-route deployment cell x=" + std::to_string(x) + " y=" + std::to_string(y) + " map="
                + std::to_string(events_.map_id) + " dimensions=" + std::to_string(width) + "x" + std::to_string(height));
        }
        return std::move(result).value();
    }
    size_t selected_role() const { return selected(); }
    size_t option() const { return option_; }
    bool select_option(size_t option)
    {
        if (done_ || equipment_ || report_ || confirmation_ || position_ || notice_ || option > 4) { return false; }
        buttons_ = true;
        option_ = option;
        activate();
        return true;
    }
    bool select_role(size_t role)
    {
        if (done_ || equipment_ || report_ || confirmation_ || position_ || notice_) { return false; }
        auto found = std::find(roles_.begin(), roles_.end(), role);
        if (found == roles_.end()) { return false; }
        first_ = size_t(found - roles_.begin());
        row_ = 0;
        buttons_ = false;
        toggle_role();
        return true;
    }
    bool select_cell(Battle::Point cell)
    {
        if (!position_ || facing_ || inspect_ || confirmation_) { return false; }
        bool available = false;
        for (size_t s = 0; s < capacity(); ++s)
        {
            if (assignment_[s] < 0 && slots_[s].x == cell.x && slots_[s].y == cell.y)
            {
                available = true;
                break;
            }
        }
        if (!available) { return false; }
        cursor_ = cell;
        choose_position();
        return facing_;
    }
    bool respond_confirmation(bool yes)
    {
        if (!confirmation_) { return false; }
        if (yes) { confirmation_->confirm(); }
        else
        {
            confirmation_->cancel();
        }
        resolve_confirmation();
        return true;
    }
    bool confirmation_pending() const { return confirmation_.has_value(); }
    void frame_presented()
    {
        if (equipment_) { equipment_->frame_presented(); }
    }
    void scan(unsigned code)
    {
        if (done_) { return; }
        if (equipment_)
        {
            auto baseline = raw_;
            equipment_->scan(code, raw_);
            if (equipment_->take_changed())
            {
                auto changed = capture_equipment();
                if (!changed)
                {
                    raw_ = std::move(baseline);
                    report_error(changed.error());
                }
            }
            if (equipment_->closed()) { equipment_.reset(); }
            return;
        }
        if (notice_)
        {
            notice_ = 0;
            return;
        }
        if (confirmation_)
        {
            if (facing_ && (code == 0x47 || code == 0x49 || code == 0x4F || code == 0x51))
            {
                auto& direction = events_.units.at(placing_).direction;
                if (code == 0x47) { direction = 3; }
                if (code == 0x49) { direction = 2; }
                if (code == 0x4F) { direction = 0; }
                if (code == 0x51) { direction = 1; }
                return;
            }
            if (code == 1) { confirmation_->cancel(); }
            if (code == 0x0F || code == 0x4B || code == 0x4D) { confirmation_->toggle(); }
            if (code == 0x1C || code == 0x39) { confirmation_->confirm(); }
            resolve_confirmation();
            return;
        }
        if (report_)
        {
            report_->event({ 1, 0, 0, 0, code });
            if (report_->page() == OriginalBattleReports::Page::closed) { report_.reset(); }
            return;
        }
        if (position_)
        {
            if (code == 1) { cancel_position(); }
            if (code == 0x4B) { map_.pan(-32, 0); }
            if (code == 0x4D) { map_.pan(32, 0); }
            if (code == 0x48) { map_.pan(0, -16); }
            if (code == 0x50) { map_.pan(0, 16); }
            if (code == 0x1C || code == 0x39)
            {
                if (inspect_) { inspect_role(); }
                else
                {
                    choose_position();
                }
            }
            return;
        }
        if (code == 0x48 || code == 0x50 || code == 0x0F) { buttons_ = !buttons_; }
        if (code == 0x4B) { navigate(false); }
        if (code == 0x4D) { navigate(true); }
        if (code == 0x1C || code == 0x39)
        {
            if (buttons_) { activate(); }
            else
            {
                toggle_role();
                if (!position_ && (deployed_count() == capacity() || deployed_count() == roles_.size()))
                {
                    buttons_ = true;
                    option_ = 2;
                }
            }
        }
    }
    void choose_position()
    {
        if (inspect_ || facing_) { return; }
        for (size_t s = 0; s < capacity(); ++s)
        {
            if (assignment_[s] < 0 && slots_[s].x == cursor_.x && slots_[s].y == cursor_.y)
            {
                auto& u = events_.units.at(placing_);
                u.x = cursor_.x;
                u.y = cursor_.y;
                u.status_flags |= 0x80;
                auto point = map_.screen_position(u.x, u.y);
                confirmation_.emplace(root_, font_, 6, 320 - graphics_.panel_width(197) / 2, std::clamp(point.second + 50, 0, 430), true);
                facing_ = true;
                break;
            }
        }
    }
    void mouse(int x, int y, bool left, bool right = false)
    {
        if (done_) { return; }
        if (equipment_)
        {
            auto baseline = raw_;
            equipment_->mouse(float(x), float(y), left, right, raw_);
            if (equipment_->take_changed())
            {
                auto updated = capture_equipment();
                if (!updated)
                {
                    raw_ = std::move(baseline);
                    report_error(updated.error());
                }
            }
            if (equipment_->closed()) { equipment_.reset(); }
            return;
        }
        if (notice_)
        {
            if (left || right) { notice_ = 0; }
            return;
        }
        if (confirmation_)
        {
            if (right) { confirmation_->cancel(); }
            else
            {
                if (facing_ && left)
                {
                    auto point = map_.screen_position(events_.units.at(placing_).x, events_.units.at(placing_).y);
                    int left_edge = point.first - 12, top = point.second - 12;
                    if (x >= left_edge - 4 && x <= left_edge + 96 && y >= top - 4 && y <= top + 60)
                    {
                        auto& direction = events_.units.at(placing_).direction;
                        if (x <= left_edge + 48 && y <= top + 28) { direction = 3; }
                        else if (y <= top + 28) { direction = 2; }
                        else if (x <= left_edge + 44) { direction = 0; }
                        else if (x <= left_edge + 92) { direction = 1; }
                    }
                }
                confirmation_->mouse(float(x), float(y), left);
            }
            resolve_confirmation();
            return;
        }
        if (report_)
        {
            report_->event({ 8, x, y, unsigned(left) | (unsigned(right) << 1) });
            if (report_->page() == OriginalBattleReports::Page::closed) { report_.reset(); }
            return;
        }
        if (position_)
        {
            if (right)
            {
                cancel_position();
                return;
            }
            map_.edge_scroll(float(x), float(y));
            cursor_ = picker_->pick_surface(float(x), float(y), map_.camera_x, map_.camera_y);
            picker_->hover(events_.units, cursor_);
            if (left)
            {
                if (inspect_) { inspect_role(); }
                else
                {
                    choose_position();
                }
            }
            return;
        }
        if (x >= 18 && x < 620 && y >= 5 && y <= 196)
        {
            size_t row = size_t((x - 18) / 122);
            if (first_ + row < roles_.size())
            {
                row_ = row;
                buttons_ = false;
                if (left) { scan(0x39); }
            }
        }
        else if (y >= 213 && y <= 243)
        {
            static constexpr std::array<std::pair<int, int>, 5> ranges{ { { 13, 119 }, { 124, 231 }, { 236, 343 }, { 349, 455 }, { 460, 567 } } };
            for (size_t i = 0; i < ranges.size(); ++i)
            {
                if (x >= ranges[i].first && x <= ranges[i].second)
                {
                    buttons_ = true;
                    option_ = i;
                    if (left) { activate(); }
                    break;
                }
            }
            if (left && x >= 572 && x <= 597)
            {
                buttons_ = false;
                navigate(false);
            }
            if (left && x >= 600 && x <= 626)
            {
                buttons_ = false;
                navigate(true);
            }
        }
    }
    Result<OriginalSave::State> prepare_commit() const
    {
        if (!done_) { return Result<OriginalSave::State>::failure(make_error("Original deployment has not been confirmed")); }
        auto captured = OriginalSaveBridge::try_capture(events_, 1, { map_.camera_x, map_.camera_y }, &raw_);
        if (!captured) { return captured; }
        auto restored = OriginalSaveBridge::try_restore(root_, captured.value());
        if (!restored) { return Result<OriginalSave::State>::failure(restored.error()); }
        auto encoded = OriginalSave::try_encode(captured.value(), captured.value().groups[0].inventory.size());
        if (!encoded) { return Result<OriginalSave::State>::failure(encoded.error()); }
        return captured;
    }
    std::vector<uint8_t> draw()
    {
        if (equipment_) { return equipment_->draw(raw_); }
        if (report_) { return report_->draw(); }
        std::vector<uint8_t> image(640 * 480 * 4);
        if (position_)
        {
            image = map_.draw();
            for (const auto& slot : slots_)
            {
                auto bitmap = placement_cell_;
                map_.mask_selection(bitmap, slot.x, slot.y);
                auto screen = map_.screen_position(slot.x, slot.y);
                for (int y = 0; y < 32; ++y)
                {
                    for (int x = 0; x < 64; ++x)
                    {
                        unsigned color = bitmap[size_t(y) * 64 + x];
                        int dx = screen.first + x, dy = screen.second + y;
                        if (!color || dx < 0 || dx >= 640 || dy < 0 || dy >= 480) { continue; }
                        size_t p = (size_t(dy) * 640 + dx) * 4;
                        for (unsigned c = 0; c < 3; ++c) { image[p + c] = uint8_t(unsigned(palette_[color * 3 + c]) * 255 / 63); }
                    }
                }
            }
            picker_->draw_selection(image, events_.units, map_.camera_x, map_.camera_y, 0, [&](auto& bitmap, auto cell)
                {
                    map_.mask_selection(bitmap, cell.x, cell.y);
                });
            map_.draw_units(image, events_.units, 1, 0);
            picker_->draw_selection(image, events_.units, map_.camera_x, map_.camera_y, 1, [&](auto& bitmap, auto cell)
                {
                    map_.mask_selection(bitmap, cell.x, cell.y);
                });
        }
        else
        {
            for (size_t offset : backgrounds_)
            {
                auto drawn = draw_original_sprite(image, cells_, palette_, offset, true, 0, 0, OriginalZeroRun::transparent);
                if (!drawn) { fatal(drawn.error()); }
            }
            for (size_t n = 0; n < 5 && first_ + n < roles_.size(); ++n)
            {
                size_t role = roles_[first_ + n];
                const auto& u = events_.units[role];
                int x = 75 + int(n) * 122;
                graphics_.role_portrait(image, u.portrait, x - 48, 8);

                font_.draw_original_large(image, u.name, x - int(u.name.size()) * 6, 142, slot_for(role) >= 0 ? 42 : 54);
                font_.draw_original_large(image, u.profession, x - int(u.profession.size()) * 6, 171, slot_for(role) >= 0 ? 42 : 54);
            }
            const auto& u = events_.units[selected()];
            auto display = std::vector<OriginalEvents::Unit>{ u };
            display[0].status_flags = 0x80;
            display[0].first_turn = 1;
            display[0].last_turn = 0;
            display[0].x = display[0].y = 1;
            auto scratch = map_.draw();
            map_.draw_units(scratch, display, 1, 0);
            auto bounds = map_.unit_bitmap_bounds(0);
            graphics_.role_bitmap(image, map_.unit_source_bitmap(0), bounds[2], bounds[3], 135 - bounds[2] / 2, 435 - bounds[3]);
            const std::array<int, 4> current{ u.hp, u.mp, u.st, u.max_action_points };
            const std::array<int, 4> limits{ u.max_hp, u.max_mp, u.max_st, u.base_action_points };
            for (size_t row = 0; row < 4; ++row)
            {
                static constexpr std::array<const char*, 4> labels{ "HP", "MP", "ST", "AP" };
                std::string label = labels[row];
                int y = 317 + int(row) * 17;
                font_.draw_small(image, { label.begin(), label.end() }, 273, y, 187, true);
                auto value = std::to_string(current[row]);
                font_.draw_small(image, { value.begin(), value.end() }, 444, y, 79, true);
                font_.draw_small(image, { '/' }, 476, y, 79, true);
                value = std::to_string(limits[row]);
                if (row == 3 && u.carry_points > 0) { value += "+" + std::to_string(u.carry_points); }
                font_.draw_small(image, { value.begin(), value.end() }, 484, y, 79, true);
                if (current[row] < limits[row] || !current[row])
                {
                    int gap = OriginalUnitStatus::missing_bar(current[row], limits[row]);
                    for (int line = 0; line < 6; ++line)
                    {
                        unsigned color = 46;
                        if (!line) { color = 221; }
                        else if (line < 3) { color = 43; }
                        for (int x = std::max(0, 419 - gap); x <= 419; ++x)
                        {
                            size_t p = (size_t(322 + int(row) * 17 + line) * 640 + x) * 4;
                            for (unsigned c = 0; c < 3; ++c) { image[p + c] = uint8_t(unsigned(palette_[color * 3 + c]) * 255 / 63); }
                        }
                    }
                }
            }
            const std::array<int, 6> values{ u.attack_limit, u.defense_limit, u.strikeback, u.sp, u.tactics, u.fencing };
            const std::array<unsigned, 6> labels{ 7, 8, 10, 11, 9, 12 };
            for (size_t n = 0; n < values.size(); ++n)
            {
                int x = 308 + int(n / 3) * 65, y = 394 + int(n % 3) * 22;
                font_.draw_small(image, font_.text_bytes(labels[n]), x, y, 54, true);
                auto value = std::to_string(values[n]);
                int offset = 0;
                if (n % 3 == 0) { offset = 1; }
                else if (n % 3 == 2) { offset = -1; }
                font_.draw_small(image, { value.begin(), value.end() }, x + 38, y + offset, 201, true);
            }
            auto level = std::to_string(u.level) + " ";
            auto level_text = std::vector<uint8_t>(level.begin(), level.end());
            auto suffix = font_.text_bytes(113);
            level_text.insert(level_text.end(), suffix.begin(), suffix.end());
            font_.draw_original_large(image, level_text, 135 - int(level_text.size()) * 6, 448, 108);
            int equipment_y = 319;
            OriginalShop items(root_);
            for (unsigned id : u.equipment)
            {
                if (!id) { continue; }
                font_.draw_small(image, items.name(id), 527, equipment_y, 56, true);
                equipment_y += 22;
            }
            for (size_t i = 0; i < 5; ++i)
            {
                auto label = font_.text_bytes(187 + unsigned(i));
                font_.draw_original_large(image, label, 65 + int(i) * 112 - int(label.size()) * 6, 217, 108);
            }
            font_.draw_original_large(image, font_.text_bytes(185), 254, 265, 42);
            font_.draw_original_large(image, font_.text_bytes(186), 449, 265, 42);
            auto ordinal = std::to_string(first_ + row_ + 1) + "/" + std::to_string(roles_.size());
            font_.draw_original_large(image, { ordinal.begin(), ordinal.end() }, 357, 265, 57);
            auto count = std::to_string(deployed_count()) + "/" + std::to_string(capacity());
            font_.draw_original_large(image, { count.begin(), count.end() }, 551, 265, 57);
            outline(image, 17 + int(row_) * 122, 5, 132 + int(row_) * 122, 197, 59);
            outline(image, 16 + int(row_) * 122, 4, 133 + int(row_) * 122, 198, 59);
            if (buttons_) { outline(image, 12 + int(option_) * 112, 213, 119 + int(option_) * 112, 243, 59); }
        }
        if (notice_)
        {
            graphics_.shared_panel(image, 0x4F72C8, 160, 88);
            font_.draw_original_large(image, font_.text_bytes(notice_), 176, 100, 42);
        }
        if (confirmation_)
        {
            if (facing_)
            {
                const auto& u = events_.units.at(placing_);
                auto point = map_.screen_position(u.x, u.y);
                picker_->draw_finish_directions(image, point.first - 12, point.second - 12, u.direction);
            }
            confirmation_->draw(image, font_);
        }
        return image;
    }
};
}    //namespace s2ne
