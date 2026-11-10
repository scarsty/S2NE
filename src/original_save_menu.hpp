#pragma once
#include "original_menu.hpp"
#include "original_save.hpp"
#include <memory>
#include <optional>
#include <string>

namespace s2ne
{
// sub_4354C0: selector only. Caller performs confirmation and actual save/load.
class OriginalSaveMenu
{
public:
    struct Slot
    {
        std::array<uint8_t, 50> description{ };
        bool occupied() const { return description[0] != 0; }
    };
    using Slots = std::array<Slot, 53>;
    enum class Action
    {
        none,
        load,
        save,
        confirm_overwrite,
        selected,
        cancel
    };
    struct Result
    {
        Action action = Action::none;
        unsigned slot = 0;
        bool changed = false;
    };
    struct Event
    {
        unsigned type = 0;
        int x = 0, y = 0;
        unsigned buttons = 0, ascii = 0, scan = 0;
    };
    static constexpr unsigned address = 0x4354C0, resource_id = 204;
    static constexpr std::array<unsigned, 4> title_ids{ 128, 129, 271, 272 };

private:
    std::vector<uint8_t> cel_, pal_;
    Slots slots_;
    int mode_, page_, row_ = 0, left_ = 0, top_ = 0, width_ = 0, height_ = 0;
    bool closed_ = false;
    std::optional<Result> pending_;
    int return_code_ = 0;
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    static s2ne::Result<std::vector<uint8_t>> try_read(const std::filesystem::path& path) { return read_resource(path); }
    static std::vector<uint8_t> read(const std::filesystem::path& path) { return try_read(path).value(); }
    size_t sprite_position() const
    {
        size_t group = dword(pal_, 816) - 57;
        return group + dword(cel_, group + 2 + (resource_id - 142) * 8);
    }
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned c) const
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
    s2ne::Result<bool> try_panel_validated(std::vector<uint8_t>& image) const
    {
        size_t p = sprite_position() + 4;
        for (int y = 0; y < height_; ++y)
        {
            int x = 0;
            while (x < width_)
            {
                unsigned op = cel_.at(p++), n = op & 127, value = 0;
                if (!op)
                {
                    n = 1;
                }
                else if (!(op & 128))
                {
                    value = cel_.at(p++);
                }
                if (!n || n > unsigned(width_ - x))
                {
                    return s2ne::Result<bool>::failure(make_error("Invalid original save-menu sprite run"));
                }
                while (n--)
                {
                    unsigned c = op & 128 ? cel_.at(p++) : value;
                    pixel(image, left_ + x++, top_ + y, c);
                }
            }
        }

        return s2ne::Result<bool>::success(true);
    }
    s2ne::Result<bool> try_panel(std::vector<uint8_t>& image) const
    {
        auto output = image;
        auto result = try_panel_validated(output);
        if (result)
        {
            image.swap(output);
        }
        return result;
    }
    void panel(std::vector<uint8_t>& image) const
    {
        auto result = try_panel(image);
        if (!result)
        {
            report_error(result.error());
        }
    }
    unsigned slot_for(int row) const { return unsigned(row < 10 ? page_ * 10 + row + 1 : row + 41); }
    bool occupied(int row) const { return slots_.at(slot_for(row) - 1).occupied(); }
    void seek_down()
    {
        // 4360C4's loop advances at most 14 times when every visible slot is empty.
        for (int i = 0; i < 14 && !occupied(row_); ++i)
        {
            row_ = (row_ + 1) % 13;
        }
    }
    Result activate()
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
            return { };
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

public:
    static Slots scan(const std::filesystem::path& root)
    {
        Slots slots{ };
        for (unsigned i = 1; i <= 53; ++i)
        {
            std::ifstream f(OriginalSave::slot_path(root, i), std::ios::binary);
            std::array<uint8_t, 9> magic{ };
            if (!f.read(reinterpret_cast<char*>(magic.data()), magic.size()) || magic != OriginalSave::signature)
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
    struct InitializationTag
    {
    };
    OriginalSaveMenu(const std::filesystem::path& root, int mode, Slots slots, int page, bool original_mode_one, InitializationTag) : cel_(read(root / "SUPERSP.CEL")), pal_(read(root / "SUPERSP.PAL")), slots_(std::move(slots)), mode_(mode), page_(page) { }
    s2ne::Result<bool> initialize(const std::filesystem::path& root, int mode, Slots slots, int page, bool original_mode_one)
    {
        if (mode < 0 || mode > 3 || page < 0 || page > 4)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid original save-menu mode/page"));
        }
        pal_.at(0) = pal_.at(1) = pal_.at(2) = 0;
        size_t p = sprite_position();
        width_ = int(word(cel_, p)) + 1;
        height_ = int(word(cel_, p + 2)) + 1;
        if (width_ > 640 || height_ > 480)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid original save-menu dimensions"));
        }
        left_ = 320 - width_ / 2;
        top_ = 240 - height_ / 2;
        if (original_mode_one && mode > 1)
        {
            closed_ = true;
        }
        return s2ne::Result<bool>::success(true);
    }
    OriginalSaveMenu(const std::filesystem::path& root, int mode, Slots slots, int page = 0, bool original_mode_one = false) : cel_(read(root / "SUPERSP.CEL")), pal_(read(root / "SUPERSP.PAL")), slots_(std::move(slots)), mode_(mode), page_(page)
    {
        auto result = initialize(root, mode, { }, page, original_mode_one);
        if (!result)
        {
            fatal(result.error());
        }
    }
    static s2ne::Result<std::unique_ptr<OriginalSaveMenu>> create(const std::filesystem::path& root, int mode, Slots slots, int page = 0, bool original_mode_one = false)
    {
        auto result = std::make_unique<OriginalSaveMenu>(root, mode, std::move(slots), page, original_mode_one, InitializationTag{ });
        auto ready = result->initialize(root, mode, { }, page, original_mode_one);
        if (!ready)
        {
            return s2ne::Result<std::unique_ptr<OriginalSaveMenu>>::failure(ready.error());
        }
        return s2ne::Result<std::unique_ptr<OriginalSaveMenu>>::success(std::move(result));
    }

    int left() const { return left_; }
    int top() const { return top_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int page() const { return page_; }
    int row() const { return row_; }
    unsigned selected_slot() const { return slot_for(row_); }
    bool closed() const { return closed_; }
    int return_code() const { return return_code_; }
    const Slots& slots() const { return slots_; }
    int hit(int x, int y) const
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
    Result command(unsigned scan)
    {
        if (closed_ || pending_)
        {
            return { };
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
    Result event(const Event& e)
    {
        if (closed_ || pending_)
        {
            return { };
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
            return { };
        }
        if (e.type != 8 && e.type != 0x20)
        {
            return { };
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
    Result confirm_overwrite(bool yes)
    {
        if (!pending_ || pending_->action != Action::confirm_overwrite)
        {
            return { };
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
    // Original closes after its synchronous IO. Adapter reports failure without claiming success.
    s2ne::Result<bool> try_complete(bool success)
    {
        if (!pending_ || pending_->action == Action::confirm_overwrite)
        {
            return s2ne::Result<bool>::failure(make_error("No original save-menu IO pending"));
        }
        if (success)
        {
            closed_ = true;
            return_code_ = pending_->action == Action::save ? 9 : pending_->slot == 51 ? 8 :
                                                                                         7;
        }
        pending_.reset();

        return s2ne::Result<bool>::success(true);
    }
    void complete(bool success)
    {
        auto result = try_complete(success);
        if (!result)
        {
            report_error(result.error());
        }
    }
    s2ne::Result<bool> try_draw_validated(std::vector<uint8_t>& image, const OriginalMenu& font) const
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
        font.draw_original_large(image, font.text_bytes(title_ids.at(mode_)), left_ + 180, top_ + 12, 73, 55);
        auto text = [&](const std::vector<uint8_t>& bytes, int x, int y, unsigned c)
        {
            font.draw_small(image, bytes, left_ + x, top_ + y, c, true);
        };
        auto ascii = [](const std::string& s)
        {
            return std::vector<uint8_t>(s.begin(), s.end());
        };
        for (int row = 0; row < 13; ++row)
        {
            unsigned slot = slot_for(row), color = row == row_ ? 175 : 229;
            int y = row < 10 ? 46 + row * 30 : 366 + (row - 10) * 30;
            text(ascii(std::to_string(slot)), 74, y, color);
            const auto& d = slots_[slot - 1].description;
            auto end = std::find(d.begin(), d.end(), uint8_t(0));
            text(d[0] ? std::vector<uint8_t>(d.begin(), end) : font.text_bytes(145), 96, y, color);
        }
        if (page_ > 0)
        {
            text(font.text_bytes(154), 86, 341, 175);
        }
        if (page_ < 4)
        {
            text(font.text_bytes(155), 150, 341, 175);
        }
        text(ascii(std::to_string(page_ + 1) + "/5"), 316, 341, 175);

        return s2ne::Result<bool>::success(true);
    }
    s2ne::Result<bool> try_draw(std::vector<uint8_t>& image, const OriginalMenu& font) const
    {
        auto output = image;
        auto result = try_draw_validated(output, font);
        if (result)
        {
            image.swap(output);
        }
        return result;
    }
    void draw(std::vector<uint8_t>& image, const OriginalMenu& font) const
    {
        auto result = try_draw(image, font);
        if (!result)
        {
            report_error(result.error());
        }
    }
};
}    //namespace s2ne
