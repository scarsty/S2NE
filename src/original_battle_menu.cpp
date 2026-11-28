#include "original_battle_menu.h"

namespace s2ne
{
auto OriginalBattleMenu::read_palette(const std::filesystem::path& p) -> SharedBytes
{
    auto palette = read_shared_resource(p).value();
    palette.mutable_at(0) = palette.mutable_at(1) = palette.mutable_at(2) = 0;
    return palette;
}

auto OriginalBattleMenu::sprite_offset(unsigned id) const -> size_t
{
    size_t g = dword(pal_, 816) - 57;
    return g + dword(cel_, g + 2 + (id - 142) * 8);
}

auto OriginalBattleMenu::sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, bool transparent) const -> void
{
    auto result = draw_original_cel(image, cel_, pal_, id, left, top, transparent ? OriginalZeroRun::transparent : OriginalZeroRun::opaque);
    if (!result)
    {
        report_error(result.error());
    }
}

auto OriginalBattleMenu::close(int code) -> void
{
    page_ = Page::closed;
    return_code_ = code;
    request_ = {};
}

auto OriginalBattleMenu::operation(Action action, unsigned address, int eax, int edx, int ebx) -> void
{
    operation_parent_ = page_;
    request_ = { action, address, eax, edx, ebx };
    page_ = Page::operation;
}

auto OriginalBattleMenu::ask(unsigned id, int code, int x, int y, bool forced) -> void
{
    confirmation_parent_ = page_;
    confirmation_id_ = id;
    confirmation_code_ = code;
    if (!settings_state_.confirm && !forced)
    {
        close(code);
        return;
    }
    confirmation_ = std::make_unique<OriginalConfirmation>(root_, font_, id, x, y, true);
    page_ = Page::confirmation;
}

auto OriginalBattleMenu::finish_confirmation() -> void
{
    if (!confirmation_->result())
    {
        return;
    }
    bool yes = *confirmation_->result();
    confirmation_.reset();
    if (yes)
    {
        close(confirmation_code_);
    }
    else
    {
        page_ = confirmation_parent_;
    }
}

auto OriginalBattleMenu::label_id(int row, Page page) const -> unsigned
{
    if (page == Page::root)
    {
        return root_ids.at(row);
    }
    if (page == Page::overview)
    {
        return overview_ids.at(row);
    }
    return system_ids.at(row);
}

auto OriginalBattleMenu::draw_page(std::vector<uint8_t>& image, Page page, int selection, bool draw_labels, bool draw_pressed) const -> void
{
    if (page == Page::root)
    {
        sprite(image, 194, left_, top_, true);
        if (context_.byte_4f6f4d == 2)
        {
            sprite(image, 196, left_ + 145, top_ + 293, false);
        }
        if (!draw_labels) { return; }
        for (int row = 0; row < (context_.byte_4f6f4d == 2 ? 6 : 5); ++row)
        {
            if (draw_pressed && row == selection)
            {
                sprite(image, 195, left_ + 145, top_ + 139 + row * 31, true);
            }
            auto bytes = font_.text_bytes(root_ids[row]);
            font_.draw_original_large(image, bytes, 320 - int(bytes.size()) * 6, top_ + 140 + row * 31, row == selection ? 119 : 233);
        }
    }
    else
    {
        sprite(image, page == Page::overview ? 201 : 200, submenu_left(), submenu_top(), true);
        unsigned title = page == Page::overview ? 122 : 127;
        auto bytes = font_.text_bytes(title);
        int x = page == Page::overview ? 320 - int(bytes.size()) * 6 : submenu_left() + 36;
        font_.draw_original_large(image, bytes, x, submenu_top() + 46, 75);
        int count = page == Page::overview ? 2 : 4;
        for (int row = 0; row < count; ++row)
        {
            font_.draw_original_large(image, font_.text_bytes(label_id(row, page)), submenu_left() + 12, submenu_top() + 77 + 31 * row, row == selection ? 119 : 233);
        }
    }
}

OriginalBattleMenu::OriginalBattleMenu(const std::filesystem::path& root, Context context, OriginalSettings::State state) : root_(root), font_(root), pal_(read_palette(root / "SUPERSP.PAL")), cel_(read_shared_resource(root / "SUPERSP.CEL").value()), palette_(pal_), context_(context), settings_state_(state)
{
    font_.small_font = state.small_font != 0;
    size_t p = sprite_offset(194);
    left_ = 320 - int(word(cel_, p) + 1) / 2;
    top_ = 240 - int(word(cel_, p + 2) + 1) / 2;
}

auto OriginalBattleMenu::page() const -> Page { return page_; }

auto OriginalBattleMenu::selected() const -> int { return selected_; }

auto OriginalBattleMenu::rows() const -> int
{
    if (page_ == Page::root)
    {
        if (context_.byte_4f6f4d == 2) { return 6; }
        else
        {
            return 5;
        }
    }
    else if (page_ == Page::overview) { return 2; }
    else if (page_ == Page::system) { return 4; }
    else
    {
        return 0;
    }
}

auto OriginalBattleMenu::left() const -> int { return left_; }

auto OriginalBattleMenu::top() const -> int { return top_; }

auto OriginalBattleMenu::submenu_left() const -> int { return left_ + 135; }

auto OriginalBattleMenu::submenu_top() const -> int { return top_ + 94; }

auto OriginalBattleMenu::request() const -> const Request& { return request_; }

auto OriginalBattleMenu::return_code() const -> std::optional<int> { return return_code_; }

auto OriginalBattleMenu::settings_state() const -> const OriginalSettings::State& { return settings_state_; }

auto OriginalBattleMenu::confirmation_id() const -> unsigned { return confirmation_id_; }

auto OriginalBattleMenu::saves_present(bool present) -> void { context_.saves_present = present; }

auto OriginalBattleMenu::try_complete_operation(int code) -> s2ne::Result<bool>
{
    if (page_ != Page::operation)
    {
        return s2ne::Result<bool>::failure(make_error("No battlefield menu operation pending"));
    }
    Action action = request_.action;
    request_ = {};
    page_ = operation_parent_;
    if (action == Action::load && code != 0)
    {
        close(code);
    }
    else if (action == Action::save && code == 9)
    {
        close(code);
    }
    else if (action == Action::inspect_characters && code == 1)
    {
        close(1);
    }

    return s2ne::Result<bool>::success(true);
}

auto OriginalBattleMenu::complete_operation(int code) -> void
{
    auto result = try_complete_operation(code);
    if (!result)
    {
        report_error(result.error());
    }
}

auto OriginalBattleMenu::hit(int x, int y) const -> int
{
    if (page_ != Page::root && page_ != Page::overview && page_ != Page::system)
    {
        return -1;
    }
    int x0 = page_ == Page::root ? left_ + 145 : submenu_left() + 7;
    int x1 = page_ == Page::root ? left_ + 245 : submenu_left() + 111;
    int y0 = page_ == Page::root ? top_ + 139 : submenu_top() + 78;
    if (x < x0 || x > x1)
    {
        return -1;
    }
    for (int row = 0; row < rows(); ++row)
    {
        if (y >= y0 + 31 * row && y <= y0 + 31 * row + 26)
        {
            return row;
        }
    }
    return -1;
}

auto OriginalBattleMenu::move(int delta) -> void
{
    int count = rows();
    if (count)
    {
        selected_ = ((selected_ + delta) % count + count) % count;
    }
}

auto OriginalBattleMenu::cancel() -> void
{
    if (page_ == Page::root)
    {
        close(0);
    }
    else if (page_ == Page::overview || page_ == Page::system)
    {
        page_ = Page::root;
        selected_ = root_selected_;
    }
    else if (page_ == Page::confirmation)
    {
        confirmation_->cancel();
        finish_confirmation();
    }
}

auto OriginalBattleMenu::activate() -> void
{
    if (page_ == Page::root)
    {
        root_selected_ = selected_;
        switch (selected_)
        {
        case 0:
            if (context_.dword_4f7cd0 != 1)
            {
                operation(Action::conditions, 0x43260D);
            }
            break;
        case 1:
            page_ = Page::overview;
            selected_ = 0;
            break;
        case 2:
            page_ = Page::system;
            selected_ = 0;
            break;
        case 3: ask(45, 6, 212, top_ + 259, false); break;
        case 4: ask(44, 3, 212, top_ + 290, false); break;
        case 5: ask(46, 15, 212, top_ + 321, false); break;
        }
    }
    else if (page_ == Page::overview)
    {
        if (selected_ == 0)
        {
            operation(Action::inspect_characters, 0x43471C, 0);
        }
        else
        {
            operation(Action::inspect_map, 0x4352CE);
        }
    }
    else if (page_ == Page::system)
    {
        system_selected_ = selected_;
        switch (selected_)
        {
        case 0:
            if (context_.saves_present)
            {
                operation(Action::load, 0x4354C0, 0, 0, 0);
                request_.eax_sprite = 204;
            }
            break;
        case 1:
            operation(Action::save, 0x4354C0, 0, 1, 0);
            request_.eax_sprite = 204;
            break;
        case 2:
            settings_ = std::make_unique<OriginalSettings>(root_, 1, settings_state_);
            page_ = Page::settings;
            break;
        case 3: ask(51, 11, submenu_left() - 44, submenu_top() + 172, true); break;
        }
    }
    else if (page_ == Page::confirmation)
    {
        confirmation_->confirm();
        finish_confirmation();
    }
}

auto OriginalBattleMenu::event(const OriginalSettings::Event& e) -> Result
{
    if (page_ == Page::settings)
    {
        auto r = settings_->event(e);
        settings_state_ = settings_->state();
        font_.small_font = settings_state_.small_font != 0;
        if (r.closed)
        {
            settings_.reset();
            page_ = Page::system;
            selected_ = system_selected_;
        }
        return { r.changed || r.closed, r.consumed_mask, r.rebuild_mask_surfaces, r.debug_scene_requested, r.save_config };
    }
    if (page_ == Page::operation || page_ == Page::closed)
    {
        return {};
    }
    if (page_ == Page::confirmation)
    {
        if (e.type == 1 || e.type == 2)
        {
            if (e.scan == 1)
            {
                cancel();
            }
            else if (e.scan == 0x4B || e.scan == 0x4D || e.scan == 0x0F)
            {
                confirmation_->toggle();
            }
            else if (e.scan == 0x1C || e.scan == 0x39)
            {
                activate();
            }
            return { true, 4 };
        }
        if (e.type == 8 || e.type == 0x20)
        {
            if (e.buttons & 2)
            {
                cancel();
            }
            else
            {
                confirmation_->mouse(float(e.x), float(e.y), (e.buttons & 1) != 0);
                finish_confirmation();
            }
            return { true, e.buttons ? 0x10u : 0u };
        }
        return {};
    }
    if (e.type == 1 || e.type == 2)
    {
        if (e.scan == 1)
        {
            cancel();
            return { true, 4 };
        }
        if (e.scan == 0x48 || e.scan == 0x50)
        {
            move(e.scan == 0x48 ? -1 : 1);
            return { true, 0 };
        }
        if (e.scan == 0x1C || e.scan == 0x39)
        {
            activate();
            return { true, 0x1F };
        }
    }
    else if (e.type == 8 || e.type == 0x20)
    {
        int row = hit(e.x, e.y);
        bool changed = row >= 0 && row != selected_;
        bool two_click = page_ == Page::overview;
        if (row >= 0)
        {
            selected_ = row;
        }
        if ((e.buttons & 1) && row >= 0 && (!two_click || !changed))
        {
            activate();
            return { true, 0x1F };
        }
        if (e.buttons & 2)
        {
            cancel();
            return { true, 0x10 };
        }
        return { changed, 0 };
    }
    return {};
}

auto OriginalBattleMenu::try_draw(const std::vector<uint8_t>& battlefield, FullWidthCanvas* canvas) const -> s2ne::Result<std::vector<uint8_t>>
{
    if (battlefield.size() != 640 * 480 * 4)
    {
        return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Battlefield menu requires 640x480 RGBA"));
    }
    if (page_ == Page::closed || page_ == Page::operation)
    {
        return s2ne::Result<std::vector<uint8_t>>::success(battlefield);
    }
    auto image = battlefield;
    auto dimmed = palette_.try_transform_rect(image, 0, 0, 640, 480, 6, canvas);
    if (!dimmed) { return s2ne::Result<std::vector<uint8_t>>::failure(dimmed.error()); }
    const bool show_root_labels = page_ == Page::root || (page_ == Page::confirmation && confirmation_parent_ == Page::root);
    draw_page(image, Page::root, page_ == Page::root ? selected_ : root_selected_, show_root_labels,
        page_ == Page::confirmation && confirmation_parent_ == Page::root);
    const Page* visiblePage;
    if (page_ == Page::confirmation) { visiblePage = &(confirmation_parent_); }
    else if (page_ == Page::operation) { visiblePage = &(operation_parent_); }
    else
    {
        visiblePage = &(page_);
    }

    Page visible = (*visiblePage);
    if (visible == Page::overview || visible == Page::system || visible == Page::settings)
    {
        draw_page(image, visible == Page::settings ? Page::system : visible, visible == Page::settings ? system_selected_ : selected_);
    }
    if (page_ == Page::settings)
    {
        return s2ne::Result<std::vector<uint8_t>>::success(settings_->draw(image));
    }
    if (page_ == Page::confirmation)
    {
        confirmation_->draw(image, font_);
    }
    return s2ne::Result<std::vector<uint8_t>>::success(image);
}

auto OriginalBattleMenu::draw(const std::vector<uint8_t>& battlefield) const -> std::vector<uint8_t>
{
    auto result = try_draw(battlefield);
    return std::move(result).value();
}
}    //namespace s2ne
