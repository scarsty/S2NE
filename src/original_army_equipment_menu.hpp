#pragma once
#include "original_item_use.hpp"
#include "original_role_panel.hpp"
#include "original_save_bridge.hpp"
#include <ctime>
namespace s2ne
{
class OriginalArmyEquipmentMenu
{
    OriginalMenu font_;
    OriginalShopMenu graphics_;
    OriginalShop items_;
    OriginalEquipment equipment_;
    OriginalRolePanel panel_;
    OriginalItemUse use_;
    bool item_mode_ = false;
    original_combat::Random random_{ 1 };
    std::optional<OriginalConfirmation> confirmation_;
    OriginalEvents events_;
    std::filesystem::path root_;
    std::vector<size_t> roles_;
    std::vector<unsigned> choices_;
    std::vector<std::vector<uint8_t>> item_help_;
    size_t first_role_ = 0, role_row_ = 0, slot_ = 0, first_ = 0, row_ = 0;
    bool closed_ = false, changed_ = false;
    unsigned fade_step_ = 16;
    bool presentation_driven_ = false;
    std::vector<uint8_t> fade(std::vector<uint8_t> image) const
    {
        if (fade_step_ >= 16) { return image; }
        unsigned intensity = std::min(fade_step_, 15u) * 16;
        for (size_t p = 0; p < image.size(); p += 4)
        {
            for (unsigned k = 0; k < 3; ++k)
            {
                unsigned dac = (unsigned(image[p + k]) * 63 + 254) / 255;
                image[p + k] = uint8_t((dac * 4) * intensity / 255);
            }
        }
        return image;
    }
    static unsigned word(const auto& b, size_t p) { return unsigned(b.at(p)) | (unsigned(b.at(p + 1)) << 8); }
    static void move(size_t& first, size_t& row, size_t count, size_t visible, bool down)
    {
        if (down && first + row + 1 < count)
        {
            if (row + 1 < visible) { ++row; }
            else
            {
                ++first;
            }
        }
        else if (!down)
        {
            if (row) { --row; }
            else if (first) { --first; }
        }
    }
    void refresh(const OriginalSave::State& state)
    {
        choices_.clear();
        if (item_mode_ && !roles_.empty()) { choices_ = use_.choices(state, selected_role()); }
        for (unsigned id = 1; !item_mode_ && id <= state.groups[0].inventory.size(); ++id)
        {
            unsigned type = equipment_.type(id);
            if (state.groups[0].inventory[id - 1] && (slot_ ? type == slot_ + 6 : type >= 1 && type <= 6)) { choices_.push_back(id); }
        }
        first_ = std::min(first_, choices_.size() > 5 ? choices_.size() - 5 : 0);
        row_ = choices_.empty() ? 0 : std::min(row_, choices_.size() - first_ - 1);
    }
    OriginalArmyEquipmentMenu(const std::filesystem::path& root, OriginalEvents events, const OriginalSave::State& state, bool item_mode, OriginalDisc disc) : font_(root), graphics_(root, disc), items_(root), equipment_(root), panel_(root, disc), use_(root), item_mode_(item_mode), events_(std::move(events)), root_(root)
    {
        for (size_t i = 0; i < state.groups[0].units.size(); ++i)
        {
            const auto& r = state.groups[0].units[i];
            if (word(r, 8) != 1 && !(r[0x33] & 0x70)) { roles_.push_back(i); }
        }
        random_.initialize(uint32_t(std::time(nullptr)));
        closed_ = roles_.empty();
        refresh(state);
        if (item_mode_ && choices_.empty()) { closed_ = true; }
    }

public:
    static Result<OriginalArmyEquipmentMenu> create(const std::filesystem::path& root, const OriginalSave::State& state, bool item_mode = false, OriginalDisc disc = OriginalDisc::one)
    {
        auto restored = OriginalSaveBridge::try_restore(root, state);
        if (!restored) { return Result<OriginalArmyEquipmentMenu>::failure(restored.error()); }
        auto help = read_resource(root / "ITEMHLSP.BIN");
        if (!help) { return Result<OriginalArmyEquipmentMenu>::failure(help.error()); }
        const auto& bytes = help.value();
        if (bytes.size() < 2) { return Result<OriginalArmyEquipmentMenu>::failure(make_error("Truncated ITEMHLSP header")); }
        std::vector<std::vector<uint8_t>> descriptions;
        size_t p = 2;
        for (unsigned i = 0; i < word(bytes, 0); ++i)
        {
            size_t end = p;
            while (end < bytes.size() && bytes[end]) { ++end; }
            if (end == bytes.size()) { return Result<OriginalArmyEquipmentMenu>::failure(make_error("Truncated ITEMHLSP description")); }
            descriptions.emplace_back(bytes.begin() + p, bytes.begin() + end);
            p = end + 1;
        }
        OriginalArmyEquipmentMenu menu(root, std::move(restored.value().events), state, item_mode, disc);
        menu.item_help_ = std::move(descriptions);
        return Result<OriginalArmyEquipmentMenu>::success(std::move(menu));
    }
    void enable_presentation_timing()
    {
        presentation_driven_ = true;
        fade_step_ = 0;
    }
    void frame_presented()
    {
        if (fade_step_ < 16) { ++fade_step_; }
    }
    bool transitioning() const { return fade_step_ < 16; }
    bool closed() const { return closed_; }
    bool take_changed() { return std::exchange(changed_, false); }
    size_t selected_role() const { return roles_.at(first_role_ + role_row_); }
    size_t selected_slot() const { return slot_; }
    size_t recipient_count() const { return roles_.size(); }
    const auto& choices() const { return choices_; }
    unsigned selected_item() const { return choices_.empty() ? 0 : choices_[first_ + row_]; }
    Result<bool> equip_selected(OriginalSave::State& state)
    {
        if (closed_ || !selected_item()) { return Result<bool>::success(false); }
        auto staged = state;
        if (!equipment_.equip(staged, selected_role(), selected_item())) { return Result<bool>::success(false); }
        auto restored = OriginalSaveBridge::try_restore(root_, staged);
        if (!restored) { return Result<bool>::failure(restored.error()); }
        events_ = std::move(restored.value().events);
        state = std::move(staged);
        changed_ = true;
        refresh(state);
        return Result<bool>::success(true);
    }
    Result<bool> unequip_selected(OriginalSave::State& state)
    {
        if (closed_) { return Result<bool>::success(false); }
        auto staged = state;
        auto removed = equipment_.try_unequip(staged, selected_role(), unsigned(slot_));
        if (!removed || !removed.value()) { return removed; }
        auto restored = OriginalSaveBridge::try_restore(root_, staged);
        if (!restored) { return Result<bool>::failure(restored.error()); }
        events_ = std::move(restored.value().events);
        state = std::move(staged);
        changed_ = true;
        refresh(state);
        return Result<bool>::success(true);
    }
    Result<bool> use_selected(OriginalSave::State& state)
    {
        auto staged = state;
        auto rng = random_;
        auto applied = use_.try_apply(staged, selected_role(), selected_item(), rng, uint32_t(std::time(nullptr)));
        if (!applied || !applied.value()) { return applied; }
        auto restored = OriginalSaveBridge::try_restore(root_, staged);
        if (!restored) { return Result<bool>::failure(restored.error()); }
        events_ = std::move(restored.value().events);
        state = std::move(staged);
        random_ = rng;
        if (presentation_driven_) { fade_step_ = 0; }
        changed_ = true;
        refresh(state);
        return Result<bool>::success(true);
    }
    void finish_confirmation(OriginalSave::State& state)
    {
        auto result = confirmation_->result();
        if (!result) { return; }
        confirmation_.reset();
        if (*result)
        {
            auto applied = use_selected(state);
            if (!applied) { report_error(applied.error()); }
        }
    }
    void scan(unsigned code, OriginalSave::State& state)
    {
        if (transitioning()) { return; }
        if (confirmation_)
        {
            if (code == 1) { confirmation_->cancel(); }
            else if (code == 0x4B || code == 0x4D || code == 0x0F) { confirmation_->toggle(); }
            else if (code == 0x1C || code == 0x39) { confirmation_->confirm(); }
            finish_confirmation(state);
            return;
        }
        if (code == 1) { closed_ = true; }
    }
    void mouse(int x, int y, bool left, bool right, OriginalSave::State& state)
    {
        if (closed_ || transitioning()) { return; }
        if (confirmation_)
        {
            if (right) { confirmation_->cancel(); }
            else
            {
                confirmation_->mouse(float(x), float(y), left);
            }
            finish_confirmation(state);
            return;
        }
        if (item_mode_)
        {
            if (right)
            {
                closed_ = true;
                return;
            }
            int top = 480 - graphics_.panel_height(229);
            if (x >= 45 && x <= 244 && y >= top + 37 && y < top + 212)
            {
                size_t row = size_t(y - top - 37) / 35;
                if (first_ + row >= choices_.size() || !use_.eligible(state, selected_role(), choices_[first_ + row])) { return; }
                bool same = row_ == row;
                row_ = row;
                if (left && same && use_.usable_in_inn(selected_item())) { confirmation_.emplace(root_, font_, 80, 200, 400, true); }
                return;
            }
            if (!left) { return; }
            if (x >= 15 && x <= 34 && y >= top + 17 && y <= top + 230)
            {
                if (y <= top + 40 || y >= top + 207) { move(first_, row_, choices_.size(), 5, y >= top + 207); }
                else if (choices_.size() > 5 && y >= top + 42 && y <= top + 205)
                {
                    first_ = std::min(size_t(y - top - 42) * choices_.size() / 163, choices_.size() - 5);
                    row_ = y >= top + 175 ? 4 : 0;
                }
                return;
            }
        }
        if (right)
        {
            closed_ = true;
            return;
        }
        const int top = 480 - graphics_.panel_height(218);
        if (!item_mode_ && x >= 269 && x <= 453 && y >= top + 52 && y < top + 227)
        {
            size_t row = size_t(y - top - 52) / 35;
            if (first_ + row >= choices_.size() || !equipment_.eligible(state.groups[0].units[selected_role()], choices_[first_ + row])) { return; }
            bool same = row_ == row;
            row_ = row;
            if (left && same)
            {
                auto result = equip_selected(state);
                if (!result) { report_error(result.error()); }
            }
            return;
        }
        if (!left) { return; }
        if (!item_mode_ && x >= 262 && x <= 459 && y >= top + 17 && y <= top + 45 && word(state.groups[0].units[selected_role()], 0xCB + slot_ * 2))
        {
            auto result = unequip_selected(state);
            if (!result) { report_error(result.error()); }
            return;
        }
        if (!item_mode_ && x >= 62 && x <= 235)
        {
            for (size_t slot = 0; slot < 6; ++slot)
            {
                if (y >= top + 22 + int(slot) * 35 && y <= top + 53 + int(slot) * 35)
                {
                    if (slot_ != slot)
                    {
                        slot_ = slot;
                        first_ = row_ = 0;
                        refresh(state);
                    }
                    return;
                }
            }
        }
        if (x >= 250 && x <= 445 && y >= 22 && y < 222)
        {
            size_t row = size_t(y - 22) / 20;
            if (first_role_ + row < roles_.size())
            {
                role_row_ = row;
                slot_ = first_ = row_ = 0;
                refresh(state);
            }
            return;
        }
        if (x >= 218 && x <= 237 && y >= 15 && y < 228)
        {
            if (y <= 38 || y >= 205) { move(first_role_, role_row_, roles_.size(), 10, y >= 205); }
            else if (roles_.size() > 10 && y >= 40 && y <= 203)
            {
                first_role_ = std::min(size_t(y - 40) * roles_.size() / 163, roles_.size() - 10);
                role_row_ = y >= 175 ? 9 : 0;
            }
            slot_ = first_ = row_ = 0;
            refresh(state);
            return;
        }
        if (!item_mode_ && x >= 239 && x <= 258 && y >= top + 51 && y <= top + 230)
        {
            if (y <= top + 74 || y >= top + 207) { move(first_, row_, choices_.size(), 5, y >= top + 207); }
            else if (choices_.size() > 5 && y >= top + 76 && y <= top + 205)
            {
                first_ = std::min(size_t(y - top - 76) * choices_.size() / 129, choices_.size() - 5);
                row_ = y >= top + 176 ? 4 : 0;
            }
        }
    }
    std::vector<uint8_t> draw(const OriginalSave::State& state) const
    {
        std::vector<uint8_t> image(640 * 480 * 4);
        for (size_t i = 3; i < image.size(); i += 4) { image[i] = 255; }
        if (closed_) { return image; }
        panel_.draw(image, events_, roles_, first_role_, role_row_, 0);
        if (item_mode_)
        {
            int top = 480 - graphics_.panel_height(229);
            graphics_.panel(image, 229, 0, top);
            for (size_t row = 0; row < 5 && first_ + row < choices_.size(); ++row)
            {
                unsigned id = choices_[first_ + row];
                unsigned color = use_.eligible(state, selected_role(), id) ? (row == row_ ? 242 : 52) : 45;
                graphics_.panel(image, id + 492, 54, top + 37 + int(row) * 35);
                font_.draw_original_large(image, items_.name(id), 88, top + 44 + int(row) * 35, color);
                if (state.groups[0].inventory[id - 1])
                {
                    auto n = std::to_string(state.groups[0].inventory[id - 1]);
                    font_.draw_original_large(image, { n.begin(), n.end() }, 213, top + 44 + int(row) * 35, color);
                }
            }
            int role_thumb = 40;
            if (first_role_ + role_row_) { role_thumb += int((first_role_ + role_row_ + 1) * 135 / roles_.size()); }
            graphics_.shared_panel(image, 0x4F72AC, 218, role_thumb);
            int thumb = top + 42;
            if (!choices_.empty() && first_ + row_) { thumb += int((first_ + row_ + 1) * 134 / choices_.size()); }
            graphics_.shared_panel(image, 0x4F72AC, 15, thumb);
            if (selected_item() && selected_item() < item_help_.size() && use_.eligible(state, selected_role(), selected_item()))
            {
                const auto& caption = item_help_[0];
                font_.draw_original_large(image, caption, 446 - int(caption.size()) * 6, top + 27, 45);
                font_.draw_original_wrapped(image, item_help_[selected_item()], 273, top + 58, 45, 7, 12, 28);
            }
            if (confirmation_) { confirmation_->draw(image, font_); }
            return fade(std::move(image));
        }
        const int top = 480 - graphics_.panel_height(218);
        graphics_.panel(image, 218, 0, top);
        const auto& role = state.groups[0].units[selected_role()];
        font_.draw_original_large(image, font_.text_bytes(75), 277, top + 20, word(role, 0xCB + slot_ * 2) ? 242 : 45);
        graphics_.outline(image, 62, top + 20 + int(slot_) * 35, 233, top + 52 + int(slot_) * 35, 60);
        font_.draw_original_large(image, font_.text_bytes(120), 503, top + 37, 45);
        int role_thumb = 40;
        if (first_role_ + role_row_) { role_thumb += int((first_role_ + role_row_ + 1) * 135 / roles_.size()); }
        graphics_.shared_panel(image, 0x4F72AC, 218, role_thumb);
        int item_thumb = top + 76;
        if (!choices_.empty() && first_ + row_) { item_thumb += int((first_ + row_ + 1) * 100 / choices_.size()); }
        graphics_.shared_panel(image, 0x4F72AC, 239, item_thumb);
        for (size_t slot = 0; slot < 6; ++slot)
        {
            unsigned id = word(role, 0xCB + slot * 2);
            if (id)
            {
                graphics_.panel(image, id + 492, 70, top + 20 + int(slot) * 35);
                font_.draw_original_large(image, items_.name(id), 104, top + 25 + int(slot) * 35, 233);
            }
        }
        for (size_t row = 0; row < 5 && first_ + row < choices_.size(); ++row)
        {
            unsigned id = choices_[first_ + row];
            unsigned color = equipment_.eligible(role, id) ? (row == row_ ? 242 : 52) : 45;
            graphics_.panel(image, id + 492, 271, top + 52 + int(row) * 35);
            font_.draw_original_large(image, items_.name(id), 305, top + 59 + int(row) * 35, color);
            auto n = std::to_string(state.groups[0].inventory[id - 1]);
            font_.draw_original_large(image, { n.begin(), n.end() }, 430, top + 59 + int(row) * 35, color);
        }
        if (selected_item() && equipment_.eligible(role, selected_item()))
        {
            unsigned old = word(role, 0xCB + slot_ * 2);
            int y = top + 80;
            std::array<unsigned, 7> offsets{ 17, 21, 22, 16, 19, 20, 23 }, labels{ 7, 8, 11, 119, 116, 117, 118 };
            for (unsigned i = 0; i < 7; ++i)
            {
                int a = equipment_.field(old, offsets[i], i == 0), b = equipment_.field(selected_item(), offsets[i], i == 0);
                if (a == b) { continue; }
                if (i == 3)
                {
                    a = (unsigned(a) & 255) % 10;
                    b = (unsigned(b) & 255) % 10;
                }
                else if (i >= 4)
                {
                    a = unsigned(a) & 255;
                    b = unsigned(b) & 255;
                }
                int value = i == 0 ? word(role, 0xC3) : i == 1 ? word(role, 0xC5) :
                                                                 a;
                auto n = std::to_string(value), next = std::to_string(i < 2 ? std::max(0, value + b - a) : b);
                font_.draw_small(image, font_.text_bytes(labels[i]), 491, y, 137, true);
                font_.draw_small(image, { n.begin(), n.end() }, 532, y, 109, true);
                graphics_.shared_panel(image, 0x4F72B0, 560, y + 4);
                font_.draw_small(image, { next.begin(), next.end() }, 588, y, a > b ? 59 : 167, true);
                y += 24;
            }
            if (y > top + 80) { font_.draw_original_large(image, font_.text_bytes(120), 503, top + 37, 233); }
        }
        return fade(std::move(image));
    }
};
}    //namespace s2ne
