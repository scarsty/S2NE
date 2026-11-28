#include "save_menu.h"

namespace s2ne
{
auto SaveMenu::Slot::occupied() const -> bool { return description[0] != 0; }

auto SaveMenu::try_read(const std::filesystem::path& path) -> s2ne::Result<SharedBytes> { return read_shared_resource(path); }

auto SaveMenu::sprite_position() const -> size_t
{
    size_t group = dword(pal_, 816) - 57;
    return group + dword(cel_, group + 2 + (resource_id - 142) * 8);
}

auto SaveMenu::pixel(std::vector<uint8_t>& image, int x, int y, unsigned c) const -> void
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480)
    {
        return;
    }
    size_t p = (size_t(y) * 640 + size_t(x)) * 4;
    for (unsigned k = 0; k < 3; ++k)
    {
        image.at(p + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
    }
    image.at(p + 3) = 255;
}

auto SaveMenu::try_panel(std::vector<uint8_t>& image) const -> s2ne::Result<bool>
{
    return draw_original_cel(image, cel_, pal_, resource_id, left_, top_, ZeroRun::transparent);
}

auto SaveMenu::panel(std::vector<uint8_t>& image) const -> void
{
    auto result = try_panel(image);
    if (!result)
    {
        report_error(result.error());
    }
}

auto SaveMenu::slot_for(int row) const -> unsigned { return unsigned(row < 10 ? page_ * 10 + row + 1 : row + 41); }

auto SaveMenu::occupied(int row) const -> bool { return slots_.at(slot_for(row) - 1).occupied(); }

auto SaveMenu::seek_down() -> void
{
    // 4360C4's loop advances at most 14 times when every visible slot is empty.
    for (int i = 0; i < 14 && !occupied(row_); ++i)
    {
        row_ = (row_ + 1) % 13;
    }
}

auto SaveMenu::activate() -> Result
{
    unsigned slot = selected_slot();
    if (mode_ == 1)
    {
        Result r{ occupied(row_) ? Action::confirm_overwrite : Action::save, slot, false };
        pending_ = r;
        return r;
    }
    if (!occupied(row_))
    {
        return {};
    }
    if (mode_ == 0)
    {
        Result r{ Action::load, slot, false };
        pending_ = r;
        return r;
    }
    closed_ = true;
    return_code_ = int(slot);
    return { Action::selected, slot, false };
}

auto SaveMenu::scan(const std::filesystem::path& root) -> Slots
{
    Slots slots{};
    for (unsigned i = 1; i <= 53; ++i)
    {
        std::ifstream f(Save::slot_path(root, i), std::ios::binary);
        std::array<uint8_t, 9> magic{};
        if (!f.read(reinterpret_cast<char*>(magic.data()), magic.size()) || magic != Save::signature)
        {
            continue;
        }
        auto& description = slots[i - 1].description;
        if (!f.read(reinterpret_cast<char*>(description.data()), description.size()))
        {
            description.fill(0);
        }
    }
    return slots;
}

SaveMenu::SaveMenu(SharedBytes cel, SharedBytes pal, int mode, Slots slots, int page) : cel_(std::move(cel)), pal_(std::move(pal)), slots_(std::move(slots)), mode_(mode), page_(page) {}

auto SaveMenu::initialize(bool original_mode_one) -> s2ne::Result<bool>
{
    if (mode_ < 0 || mode_ > 3 || page_ < 0 || page_ > 4)
    {
        return s2ne::Result<bool>::failure(make_error("Invalid original save-menu mode/page"));
    }
    pal_.mutable_at(0) = pal_.mutable_at(1) = pal_.mutable_at(2) = 0;
    ResourceReader palette(pal_), archive(cel_);
    const unsigned address = palette.number(816, 4);
    if (!palette || address < 57) { return s2ne::Result<bool>::failure(make_error("Invalid original save-menu directory")); }
    const size_t group = address - 57;
    const size_t p = group + archive.number(group + 2 + (resource_id - 142) * 8, 4);
    width_ = int(archive.number(p)) + 1;
    height_ = int(archive.number(p + 2)) + 1;
    if (!archive) { return s2ne::Result<bool>::failure(archive.error()); }
    if (width_ > 640 || height_ > 480)
    {
        return s2ne::Result<bool>::failure(make_error("Invalid original save-menu dimensions"));
    }
    left_ = 320 - width_ / 2;
    top_ = 240 - height_ / 2;
    if (original_mode_one && mode_ > 1)
    {
        closed_ = true;
    }
    return s2ne::Result<bool>::success(true);
}

SaveMenu::SaveMenu(const std::filesystem::path& root, int mode, Slots slots, int page, bool original_mode_one) : SaveMenu(std::move(*create(root, mode, std::move(slots), page, original_mode_one).value()))
{
}

auto SaveMenu::create(const std::filesystem::path& root, int mode, Slots slots, int page, bool original_mode_one) -> s2ne::Result<std::unique_ptr<SaveMenu>>
{
    auto cel = try_read(root / "SUPERSP.CEL");
    if (!cel) { return s2ne::Result<std::unique_ptr<SaveMenu>>::failure(cel.error()); }
    auto pal = try_read(root / "SUPERSP.PAL");
    if (!pal) { return s2ne::Result<std::unique_ptr<SaveMenu>>::failure(pal.error()); }
    if (pal.value().size() < 820) { return s2ne::Result<std::unique_ptr<SaveMenu>>::failure(make_error("Truncated save-menu palette")); }
    auto result = std::unique_ptr<SaveMenu>(new SaveMenu(std::move(cel).value(), std::move(pal).value(), mode, std::move(slots), page));
    auto ready = result->initialize(original_mode_one);
    if (!ready)
    {
        return s2ne::Result<std::unique_ptr<SaveMenu>>::failure(ready.error());
    }
    return s2ne::Result<std::unique_ptr<SaveMenu>>::success(std::move(result));
}

auto SaveMenu::left() const -> int { return left_; }

auto SaveMenu::top() const -> int { return top_; }

auto SaveMenu::width() const -> int { return width_; }

auto SaveMenu::height() const -> int { return height_; }

auto SaveMenu::page() const -> int { return page_; }

auto SaveMenu::row() const -> int { return row_; }

auto SaveMenu::selected_slot() const -> unsigned { return slot_for(row_); }

auto SaveMenu::closed() const -> bool { return closed_; }

auto SaveMenu::return_code() const -> int { return return_code_; }

auto SaveMenu::slots() const -> const Slots& { return slots_; }

auto SaveMenu::hit(int x, int y) const -> int
{
    x -= left_;
    y -= top_;
    if (x < 74 || x > 363)
    {
        return -1;
    }
    if (y >= 41 && y < 341)
    {
        return (y - 41) / 30;
    }
    if (mode_ != 1 && y >= 361 && y < 451)
    {
        return 10 + (y - 361) / 30;
    }
    return -1;
}

auto SaveMenu::command(unsigned scan) -> Result
{
    if (closed_ || pending_)
    {
        return {};
    }
    if (scan == 1)
    {
        closed_ = true;
        return_code_ = 0;
        return { Action::cancel, 0, false };
    }
    if (scan == 0x39)
    {
        return activate();
    }
    int old_row = row_, old_page = page_;
    if (scan == 0x48 || scan == 0x50)
    {
        int delta = scan == 0x48 ? -1 : 1, limit = mode_ == 1 ? 10 : 13;
        if (mode_ == 1)
        {
            row_ = (row_ + delta + limit) % limit;
        }
        else
        {
            for (int i = 0; i < 14; ++i)
            {
                row_ = (row_ + delta + limit) % limit;
                if (occupied(row_))
                {
                    break;
                }
            }
        }
    }
    else if (scan == 0x49 || scan == 0x51)
    {
        page_ = std::clamp(page_ + (scan == 0x49 ? -1 : 1), 0, 4);
        if (page_ != old_page && mode_ != 1)
        {
            seek_down();
        }
    }
    return { Action::none, 0, row_ != old_row || page_ != old_page };
}

auto SaveMenu::event(const Event& e) -> Result
{
    if (closed_ || pending_)
    {
        return {};
    }
    if (e.type == 1 || e.type == 2 || e.type == 0x2E)
    {
        if (e.ascii == 13 || e.ascii == 32)
        {
            return command(0x39);
        }
        if (e.ascii == 27)
        {
            return command(1);
        }
        if (!e.ascii)
        {
            return command(e.scan);
        }
        return {};
    }
    if (e.type != 8 && e.type != 0x20)
    {
        return {};
    }
    Result r;
    int x = e.x - left_, y = e.y - top_;
    if (y >= 340 && y <= 360)
    {
        if (e.buttons & 1)
        {
            if (x >= 84 && x <= 136)
            {
                r = command(0x49);
            }
            else if (x >= 150 && x <= 200)
            {
                r = command(0x51);
            }
        }
    }
    else
    {
        int row = hit(e.x, e.y);
        if (row >= 0 && (mode_ == 1 || occupied(row)))
        {
            if (row_ != row)
            {
                row_ = row;
                r.changed = true;
            }
            else if (e.buttons & 1)
            {
                r = activate();
            }
        }
    }
    if (e.buttons & 2)
    {
        pending_.reset();
        closed_ = true;
        return_code_ = 0;
        return { Action::cancel, 0, r.changed };
    }
    return r;
}

auto SaveMenu::confirm_overwrite(bool yes) -> Result
{
    if (!pending_ || pending_->action != Action::confirm_overwrite)
    {
        return {};
    }
    auto r = *pending_;
    pending_.reset();
    if (!yes)
    {
        return { Action::none, 0, true };
    }
    r.action = Action::save;
    pending_ = r;
    return r;
}

auto SaveMenu::try_complete(bool success) -> s2ne::Result<bool>
{
    if (!pending_ || pending_->action == Action::confirm_overwrite)
    {
        return s2ne::Result<bool>::failure(make_error("No original save-menu IO pending"));
    }
    if (success)
    {
        closed_ = true;
        int completionCode;
        if (pending_->action == Action::save) { completionCode = 9; }
        else if (pending_->slot == 51) { completionCode = 8; }
        else
        {
            completionCode = 7;
        }

        return_code_ = completionCode;
    }
    pending_.reset();

    return s2ne::Result<bool>::success(true);
}

auto SaveMenu::complete(bool success) -> void
{
    auto result = try_complete(success);
    if (!result)
    {
        report_error(result.error());
    }
}

auto SaveMenu::try_draw_validated(std::vector<uint8_t>& image, const Menu& font) const -> s2ne::Result<bool>
{
    if (image.size() != 640 * 480 * 4)
    {
        return s2ne::Result<bool>::failure(make_error("Invalid save-menu framebuffer"));
    }
    auto panel_result = try_panel(image);
    if (!panel_result)
    {
        return panel_result;
    }
    auto title = font.try_draw_original_large(image, font.text_bytes(title_ids.at(mode_)), left_ + 180, top_ + 12, 73, 55);
    if (!title) { return title; }
    auto text = [&](const std::vector<uint8_t>& bytes, int x, int y, unsigned c)
    {
        return font.try_draw_small_fixed_spaces(image, bytes, left_ + x, top_ + y, c, true);
    };
    auto ascii = [](const std::string& s)
    {
        return std::vector<uint8_t>(s.begin(), s.end());
    };
    for (int row = 0; row < 13; ++row)
    {
        unsigned slot = slot_for(row), color = row == row_ ? 175 : 229;
        int y = row < 10 ? 46 + row * 30 : 366 + (row - 10) * 30;
        auto number = text(ascii(std::to_string(slot)), 74, y, color);
        if (!number) { return number; }
        const auto& d = slots_[slot - 1].description;
        auto end = std::find(d.begin(), d.end(), uint8_t(0));
        auto description = text(d[0] ? std::vector<uint8_t>(d.begin(), end) : font.text_bytes(145), 96, y, color);
        if (!description) { return description; }
    }
    if (page_ > 0)
    {
        auto previous = text(font.text_bytes(154), 86, 341, 175);
        if (!previous) { return previous; }
    }
    if (page_ < 4)
    {
        auto next = text(font.text_bytes(155), 150, 341, 175);
        if (!next) { return next; }
    }
    return text(ascii(std::to_string(page_ + 1) + "/5"), 316, 341, 175);
}

auto SaveMenu::try_draw(std::vector<uint8_t>& image, const Menu& font) const -> s2ne::Result<bool>
{
    auto output = image;
    auto result = try_draw_validated(output, font);
    if (result)
    {
        image.swap(output);
    }
    return result;
}

auto SaveMenu::draw(std::vector<uint8_t>& image, const Menu& font) const -> void
{
    auto result = try_draw(image, font);
    if (!result)
    {
        report_error(result.error());
    }
}
}    //namespace s2ne
