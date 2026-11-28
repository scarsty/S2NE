#pragma once
#include "item_use.h"
#include "role_panel.h"
#include "save_bridge.h"
#include <ctime>
namespace s2ne
{
class ArmyEquipmentMenu
{
    Menu font_;
    ShopMenu graphics_;
    Shop items_;
    Equipment equipment_;
    RolePanel panel_;
    ItemUse use_;
    bool item_mode_ = false;
    std::function<Result<bool>(size_t, unsigned)> use_adapter_;
    std::optional<size_t> fixed_role_;
    s2ne::Random random_{ 1 };
    std::optional<Confirmation> confirmation_;
    Events events_;
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
    void refresh(const Save::State& state);
    ArmyEquipmentMenu(const std::filesystem::path& root, Events events, const Save::State& state, bool item_mode, Disc disc);

public:
    static Result<ArmyEquipmentMenu> create(const std::filesystem::path& root, const Save::State& state, bool item_mode = false, Disc disc = Disc::one);
    void enable_presentation_timing();
    void frame_presented();
    bool transitioning() const;
    bool closed() const;
    void set_use_adapter(size_t role, std::function<Result<bool>(size_t, unsigned)> adapter);
    void refresh_items(const Save::State& state);
    void return_from_target(const Save::State& state);
    bool take_changed();
    size_t selected_role() const;
    size_t selected_slot() const;
    size_t recipient_count() const;
    const std::vector<unsigned int>& choices() const;
    unsigned selected_item() const;
    bool select_item(unsigned id);
    bool confirm_use(Save::State& state);
    Result<bool> equip_selected(Save::State& state);
    Result<bool> unequip_selected(Save::State& state);
    Result<bool> use_selected(Save::State& state);
    void finish_confirmation(Save::State& state);
    void scan(unsigned code, Save::State& state);
    void mouse(int x, int y, bool left, bool right, Save::State& state);
    std::vector<uint8_t> draw(const Save::State& state, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
