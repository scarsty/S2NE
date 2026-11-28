#include "original_inn.h"

namespace s2ne
{
OriginalInn::OriginalInn(const std::filesystem::path& root, OriginalDisc disc) : font_(root), background_(root, disc, 13), graphics_(root, disc), root_(root), save_root_(root), domain_(disc)
{
    auto result = try_load_towns(root);
    if (!result)
    {
        report_error(result.error());
        closed_ = true;
        return;
    }
    auto towns = std::move(result).value();
    towns_ = std::move(towns);
    settings_initial_ = OriginalSettings::load_state(root_);
    random_.initialize(uint32_t(std::time(nullptr)));
}

auto OriginalInn::try_load_towns(const std::filesystem::path& root) -> Result<std::vector<uint8_t>>
{
    auto json_towns = try_load_towns_json(root);
    if (!json_towns)
    {
        return Result<std::vector<uint8_t>>::failure(json_towns.error());
    }
    if (!json_towns.value().empty()) { return json_towns; }
    auto pal = read_shared_resource(root / "SUPERSP.PAL"), bin = read_shared_resource(root / "SUPERSP.BIN");
    if (!pal)
    {
        return Result<std::vector<uint8_t>>::failure(pal.error());
    }
    if (!bin)
    {
        return Result<std::vector<uint8_t>>::failure(bin.error());
    }
    ResourceReader reader(pal.value());
    auto address = reader.number(872, 4), n = reader.number(876, 4);
    if (!reader)
    {
        return Result<std::vector<uint8_t>>::failure(reader.error());
    }
    if (address < 14)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid original inn BIN4 address"));
    }
    size_t p = address - 14;
    if (p > bin.value().size() || n < 14 || n - 14 > bin.value().size() - p)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid original inn BIN4"));
    }
    return Result<std::vector<uint8_t>>::success({ bin.value().begin() + p, bin.value().begin() + p + n - 14 });
}

auto OriginalInn::closed() const -> bool { return closed_; }

auto OriginalInn::select_auto_equipment() -> bool
{
    if (closed_ || reading_ || roster_ || equipment_menu_ || world_ || settings_ || auto_confirm_ || title_confirm_ || saves_ || overwrite_) { return false; }
    selected_ = 10;
    auto_confirm_.emplace(root_, font_, 6, 200, 200, true);
    return true;
}

auto OriginalInn::confirm_auto_equipment(OriginalSave::State& state) -> bool
{
    if (!auto_confirm_) { return false; }
    auto_confirm_->confirm();
    finish_auto(state);
    return blocker_.empty();
}

auto OriginalInn::leave_root() -> bool
{
    if (closed_ || reading_ || roster_ || equipment_menu_ || world_ || settings_ || auto_confirm_ || title_confirm_ || saves_ || overwrite_) { return false; }
    closed_ = true;
    return true;
}

auto OriginalInn::set_save_root(const std::filesystem::path& root) -> void { save_root_ = root; }

auto OriginalInn::title_requested() const -> bool { return title_requested_; }

auto OriginalInn::title_confirmation_active() const -> bool { return title_confirm_.has_value(); }

auto OriginalInn::finish_title_confirmation() -> void
{
    if (auto yes = title_confirm_->result())
    {
        title_requested_ = *yes;
        closed_ = *yes;
        title_confirm_.reset();
    }
}

auto OriginalInn::reading() const -> bool { return reading_; }

auto OriginalInn::roster_active() const -> bool { return roster_.has_value(); }

auto OriginalInn::equipment_active() const -> bool { return equipment_menu_.has_value(); }

auto OriginalInn::transitioning() const -> bool { return equipment_menu_ && equipment_menu_->transitioning(); }

auto OriginalInn::world_active() const -> bool { return world_.has_value(); }

auto OriginalInn::tick(unsigned clock) -> void
{
    if (world_)
    {
        world_->tick(clock);
        if (world_->closed())
        {
            world_.reset();
        }
    }
}

auto OriginalInn::enable_world_presentation() -> void
{
    if (world_)
    {
        world_->enable_presentation_timing();
    }
}

auto OriginalInn::enable_presentation_timing() -> void { presentation_driven_ = true; }

auto OriginalInn::frame_presented() -> void
{
    if (equipment_menu_) { equipment_menu_->frame_presented(); }
    if (world_)
    {
        world_->frame_presented();
    }
}

auto OriginalInn::worldmap() const -> const OriginalWorldmap* { return world_ ? &*world_ : nullptr; }

auto OriginalInn::rumor() const -> const std::vector<unsigned char>& { return rumor_; }

auto OriginalInn::blocker() const -> const std::string& { return blocker_; }

auto OriginalInn::town_rumor(const OriginalSave::State& state, unsigned branch) const -> std::vector<uint8_t>
{
    unsigned town = unsigned(state.globals_4f7cd0.at(24)) | (unsigned(state.globals_4f7cd0.at(25)) << 8);
    if (!town || town > word(towns_, 0))
    {
        return {};
    }
    size_t record = 2 + size_t(town - 1) * 14, p = dword(towns_, record);
    while (towns_.at(p))
    {
        ++p;
    }
    ++p;
    p += 2u * (towns_.at(record + 11) + towns_.at(record + 12) + towns_.at(record + 13));
    if (branch)
    {
        while (towns_.at(p))
        {
            ++p;
        }
        ++p;
    }
    size_t begin = p;
    while (towns_.at(p))
    {
        ++p;
    }
    return { towns_.begin() + begin, towns_.begin() + p };
}

auto OriginalInn::take_settings() -> std::optional<OriginalSettings::State>
{
    auto result = settings_result_;
    settings_result_.reset();
    return result;
}

auto OriginalInn::prepare_settings(const OriginalSettings::State& state) -> void { settings_initial_ = state; }

auto OriginalInn::settings_active() const -> bool { return settings_.has_value(); }

auto OriginalInn::settings_event(const OriginalSettings::Event& event) -> void
{
    auto result = settings_->event(event);
    if (result.closed)
    {
        settings_result_ = settings_->state();
        settings_.reset();
    }
}

auto OriginalInn::set_save_writer(std::function<Result<OriginalSave::State>(unsigned, const OriginalSave::State&)> writer) -> void { save_writer_ = std::move(writer); }

auto OriginalInn::take_loaded() -> std::optional<OriginalSaveBridge::Restored>
{
    auto result = std::move(loaded_);
    loaded_.reset();
    return result;
}

auto OriginalInn::saves_active() const -> bool { return saves_.has_value(); }

auto OriginalInn::save_event(const OriginalSaveMenu::Event& event, const OriginalSave::State& state) -> void
{
    OriginalSaveMenu::Result result;
    if (overwrite_)
    {
        if (event.type == 1)
        {
            if (event.scan == 1)
            {
                overwrite_->cancel();
            }
            else if (event.scan == 0x4B || event.scan == 0x4D)
            {
                overwrite_->toggle();
            }
            else if (event.scan == 0x1C || event.scan == 0x39)
            {
                overwrite_->confirm();
            }
        }
        else if (event.type == 0x20)
        {
            if (event.buttons & 2)
            {
                overwrite_->cancel();
            }
            else
            {
                overwrite_->mouse(float(event.x), float(event.y), (event.buttons & 1) != 0);
            }
        }
        if (auto yes = overwrite_->result())
        {
            result = saves_->confirm_overwrite(*yes);
            overwrite_.reset();
        }
    }
    else
    {
        result = saves_->event(event);
    }
    if (result.action == OriginalSaveMenu::Action::confirm_overwrite)
    {
        overwrite_.emplace(root_, font_, 253, 176, 76 + 30 * saves_->row(), true);
    }
    if (result.action == OriginalSaveMenu::Action::save || result.action == OriginalSaveMenu::Action::load)
    {
        auto count = state.groups[0].inventory.size();
        if (result.action == OriginalSaveMenu::Action::save)
        {
            if (!save_writer_)
            {
                blocker_ = "Original inn save writer unavailable";
                report_error(make_error(blocker_));
                saves_->complete(false);
                return;
            }
            auto written = save_writer_(result.slot, state);
            saves_->complete(bool(written));
            if (!written) { report_error(written.error()); }
        }
        else
        {
            auto loaded = OriginalSave::try_load(OriginalSave::slot_path(save_root_, result.slot), count);
            if (loaded)
            {
                auto restored = OriginalSaveBridge::try_restore(root_, loaded.value());
                if (!restored)
                {
                    blocker_ = restored.error().message;
                    report_error(restored.error());
                    saves_->complete(false);
                    return;
                }
                loaded_ = std::move(restored).value();
                saves_->complete(true);
            }
            else
            {
                saves_->complete(false);
                report_error(loaded.error());
            }
        }
    }
    if (saves_->closed())
    {
        saves_.reset();
    }
}

auto OriginalInn::take_equipment_changed() -> bool
{
    bool result = equipment_changed_;
    equipment_changed_ = false;
    return result;
}

auto OriginalInn::finish_auto(OriginalSave::State& state) -> void
{
    if (auto yes = auto_confirm_->result())
    {
        auto_confirm_.reset();
        if (*yes)
        {
            auto equipped = OriginalEquipment(root_).try_auto_equip(state);
            if (!equipped)
            {
                blocker_ = equipped.error().message;
                report_error(equipped.error());
            }
            else
            {
                equipment_changed_ = true;
            }
        }
    }
}

auto OriginalInn::scan(unsigned code, OriginalSave::State& state) -> void
{
    if (title_confirm_)
    {
        if (code == 1) { title_confirm_->cancel(); }
        else if (code == 0x4B || code == 0x4D || code == 0x0F) { title_confirm_->toggle(); }
        else if (code == 0x1C || code == 0x39) { title_confirm_->confirm(); }
        finish_title_confirmation();
        return;
    }
    if (world_)
    {
        world_->scan(code);
        if (world_->closed())
        {
            world_.reset();
        }
        return;
    }
    if (auto_confirm_)
    {
        if (code == 1)
        {
            auto_confirm_->cancel();
        }
        else if (code == 0x4B || code == 0x4D || code == 0x0F)
        {
            auto_confirm_->toggle();
        }
        else if (code == 0x1C || code == 0x39)
        {
            auto_confirm_->confirm();
        }
        finish_auto(state);
        return;
    }
    if (saves_)
    {
        OriginalSaveMenu::Event event;
        event.type = 1;
        event.scan = code;
        event.ascii = code == 0x1C ? 13 : 0;
        save_event(event, state);
        return;
    }
    if (settings_)
    {
        OriginalSettings::Event event;
        event.type = 1;
        event.scan = code;
        settings_event(event);
        return;
    }
    if (equipment_menu_)
    {
        equipment_menu_->scan(code, state);
        equipment_changed_ |= equipment_menu_->take_changed();
        if (equipment_menu_->closed()) { equipment_menu_.reset(); }
        return;
    }
    if (roster_)
    {
        OriginalSettings::Event event;
        event.type = 1;
        event.scan = code;
        roster_->event(event);
        if (roster_->page() == OriginalBattleReports::Page::closed)
        {
            roster_.reset();
        }
        return;
    }
    if (closed_)
    {
        return;
    }
    if (reading_)
    {
        if (code)
        {
            reading_ = false;
        }
        return;
    }
    if (code == 1)
    {
        closed_ = true;
        return;
    }
    if (code == 0x4B || code == 0x4D || code == 0x0F)
    {
        selected_ = selected_ % 10 + 1;
    }
    if (code != 0x1C && code != 0x39)
    {
        return;
    }
    blocker_.clear();
    if (selected_ == 1)
    {
        auto world = OriginalWorldmap::create(root_, domain_, towns_, state);
        if (!world)
        {
            blocker_ = world.error().message;
            return;
        }
        world_ = std::move(world).value();
        return;
    }
    if (selected_ == 3 || selected_ == 4)
    {
        auto slots = OriginalSaveMenu::scan(save_root_);
        if (selected_ == 3 && !std::any_of(slots.begin(), slots.end(), [](const auto& slot)
                {
                    return slot.occupied();
                }))
        {
            return;
        }
        saves_.emplace(root_, selected_ == 4 ? 1 : 0, std::move(slots), 0, true);
        return;
    }
    if (selected_ == 8)
    {
        title_confirm_.emplace(root_, font_, 51, 300, 200, true);
        return;
    }
    if (selected_ == 10)
    {
        auto_confirm_.emplace(root_, font_, 6, 200, 200, true);
        return;
    }
    if (selected_ == 9)
    {
        settings_.emplace(root_, 2, settings_initial_);
        return;
    }
    if (selected_ == 5 || selected_ == 7)
    {
        auto menu = OriginalArmyEquipmentMenu::create(root_, state, selected_ == 7, domain_);
        if (!menu)
        {
            blocker_ = menu.error().message;
            return;
        }
        if (!menu.value().closed())
        {
            equipment_menu_ = std::move(menu).value();
            if (presentation_driven_) { equipment_menu_->enable_presentation_timing(); }
        }
        return;
    }
    if (selected_ == 6)
    {
        auto result = OriginalSaveBridge::try_restore(root_, state);
        if (!result)
        {
            blocker_ = result.error().message;
            return;
        }
        auto restored = std::move(result).value();
        std::vector<OriginalBattleReports::Entry> entries;
        for (size_t i = 0; i < state.groups[0].units.size(); ++i)
        {
            const auto& u = restored.events.units.at(i);
            const auto& role = state.groups[0].units[i];
            unsigned kind = unsigned(role[8]) | (unsigned(role[9]) << 8);
            if (!(u.status_flags & 0x70) && kind != 1)
            {
                entries.push_back({ u, false, int(u.runtime_role_selector), true });
            }
        }
        roster_.emplace(root_, OriginalBattleReports::Page::roster, restored.events, std::move(entries), background_.background());
        return;
    }
    if (selected_ != 2)
    {
        blocker_ = "410EEE subpage not restored; option10=451ABC auto-equipment, not lodging";
        return;
    }
    auto text = town_rumor(state, unsigned(random_.bounded(2, uint32_t(std::time(nullptr)))));
    if (text.empty())
    {
        blocker_ = "414C4A original town/rumor unavailable";
        return;
    }
    rumor_ = std::move(text);
    reading_ = true;
}

auto OriginalInn::mouse(int x, int y, bool left, bool right, OriginalSave::State& state) -> void
{
    if (title_confirm_)
    {
        if (right) { title_confirm_->cancel(); }
        else
        {
            title_confirm_->mouse(float(x), float(y), left);
        }
        finish_title_confirmation();
        return;
    }
    if (world_)
    {
        world_->mouse(x, y, left, right);
        if (world_->closed())
        {
            world_.reset();
        }
        return;
    }
    if (auto_confirm_)
    {
        if (right)
        {
            auto_confirm_->cancel();
        }
        else
        {
            auto_confirm_->mouse(float(x), float(y), left);
        }
        finish_auto(state);
        return;
    }
    if (saves_)
    {
        OriginalSaveMenu::Event event;
        event.type = left || right ? 0x20 : 8;
        event.x = x;
        event.y = y;
        int mouseButtons;
        if (left) { mouseButtons = 1; }
        else if (right) { mouseButtons = 2; }
        else
        {
            mouseButtons = 0;
        }

        event.buttons = mouseButtons;
        save_event(event, state);
        return;
    }
    if (settings_)
    {
        OriginalSettings::Event event;
        event.type = left || right ? 0x20 : 8;
        event.x = x;
        event.y = y;
        int mouseButtons;
        if (left) { mouseButtons = 1; }
        else if (right) { mouseButtons = 2; }
        else
        {
            mouseButtons = 0;
        }

        event.buttons = mouseButtons;
        settings_event(event);
        return;
    }
    if (equipment_menu_)
    {
        equipment_menu_->mouse(x, y, left, right, state);
        equipment_changed_ |= equipment_menu_->take_changed();
        if (equipment_menu_->closed()) { equipment_menu_.reset(); }
        return;
    }
    if (roster_)
    {
        OriginalSettings::Event event;
        event.type = left || right ? 0x20 : 8;
        event.x = x;
        event.y = y;
        int mouseButtons;
        if (left) { mouseButtons = 1; }
        else if (right) { mouseButtons = 2; }
        else
        {
            mouseButtons = 0;
        }

        event.buttons = mouseButtons;
        roster_->event(event);
        if (roster_->page() == OriginalBattleReports::Page::closed)
        {
            roster_.reset();
        }
        return;
    }
    if (right)
    {
        scan(1, state);
        return;
    }
    if (reading_)
    {
        if (left)
        {
            scan(0x1C, state);
        }
        return;
    }
    for (unsigned i = 0; i < 10; ++i)
    {
        auto r = rectangles[i];
        if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom)
        {
            selected_ = i + 1;
            if (left)
            {
                scan(0x1C, state);
            }
        }
    }
}

auto OriginalInn::draw(const OriginalSave::State& state, FullWidthCanvas* canvas) const -> std::vector<uint8_t>
{
    if (world_)
    {
        return world_->draw(font_);
    }
    if (settings_)
    {
        return settings_->draw(background_.background());
    }
    if (equipment_menu_)
    {
        return equipment_menu_->draw(state, canvas);
    }
    if (roster_)
    {
        return roster_->draw();
    }
    if (reading_)
    {
        auto image = graphics_.service_panel(154, 194, OriginalShopMenu::money(state), background_, false);
        font_.draw_original_wrapped(image, rumor_, 149, 42, 42, 6, 16, 28);
        return image;
    }
    auto image = background_.background();
    if (title_confirm_)
    {
        title_confirm_->draw(image, font_);
        return image;
    }
    if (auto_confirm_)
    {
        auto_confirm_->draw(image, font_);
        return image;
    }
    if (saves_)
    {
        saves_->draw(image, font_);
        if (overwrite_)
        {
            overwrite_->draw(image, font_);
        }
        return image;
    }
    auto r = rectangles[selected_ - 1];
    auto label = font_.text_bytes(labels[selected_ - 1]);
    background_.draw_caption(image, labels[selected_ - 1], (r.left + r.right) / 2 - int(label.size()) * 4, (r.top + r.bottom) / 2 - 30);
    return image;
}
}    //namespace s2ne
