#pragma once
#include "original_item_use.h"
#include "original_role_panel.h"
#include "original_save_bridge.h"
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
    std::function<Result<bool>(size_t, unsigned)> use_adapter_;
    std::optional<size_t> fixed_role_;
    s2ne::Random random_{ 1 };
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
    std::vector<uint8_t> fade(std::vector<uint8_t> image, FullWidthCanvas* canvas) const;
    static void move(size_t& first, size_t& row, size_t count, size_t visible, bool down);
    void refresh(const OriginalSave::State& state);
    OriginalArmyEquipmentMenu(const std::filesystem::path& root, OriginalEvents events, const OriginalSave::State& state, bool item_mode, OriginalDisc disc);

public:
    static Result<OriginalArmyEquipmentMenu> create(const std::filesystem::path& root, const OriginalSave::State& state, bool item_mode = false, OriginalDisc disc = OriginalDisc::one);
    void enable_presentation_timing();
    void frame_presented();
    bool transitioning() const;
    bool closed() const;
    void set_use_adapter(size_t role, std::function<Result<bool>(size_t, unsigned)> adapter);
    void refresh_items(const OriginalSave::State& state);
    void return_from_target(const OriginalSave::State& state);
    bool take_changed();
    size_t selected_role() const;
    size_t selected_slot() const;
    size_t recipient_count() const;
    const std::vector<unsigned int>& choices() const;
    unsigned selected_item() const;
    bool select_item(unsigned id);
    bool confirm_use(OriginalSave::State& state);
    Result<bool> equip_selected(OriginalSave::State& state);
    Result<bool> unequip_selected(OriginalSave::State& state);
    Result<bool> use_selected(OriginalSave::State& state);
    void finish_confirmation(OriginalSave::State& state);
    void scan(unsigned code, OriginalSave::State& state);
    void mouse(int x, int y, bool left, bool right, OriginalSave::State& state);
    std::vector<uint8_t> draw(const OriginalSave::State& state, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
