#pragma once
#include "army_equipment_menu.h"
#include "battle.h"
#include "battle_reports.h"
#include "wide_viewport.h"

namespace s2ne
{
// 原44AED2在出阵确认前只修改候选军团，不发布存档状态。
class Deployment
{
    std::filesystem::path root_;
    Disc disc_;
    Menu font_;
    ShopMenu graphics_;
    Map map_;
    std::optional<Battle> picker_;
    Events events_;
    Save::State raw_;
    std::vector<size_t> roles_;
    std::vector<Events::DeploymentSlot> slots_;
    std::vector<int> assignment_;
    std::vector<bool> fixed_;
    std::vector<uint8_t> cells_, palette_;
    std::vector<uint8_t> placement_cell_;
    std::array<size_t, 2> backgrounds_{};
    size_t first_{}, row_{}, option_{}, placing_{};
    bool buttons_{}, position_{}, facing_{}, done_{}, inspect_{};
    unsigned notice_{};
    std::optional<Confirmation> confirmation_;
    std::optional<ArmyEquipmentMenu> equipment_;
    std::optional<BattleReports> report_;
    std::optional<Events::Unit> before_position_;
    int before_slot_{ -1 };
    Battle::Point cursor_{ -1, -1 };
    size_t selected() const;
    size_t deployed_count() const;
    int slot_for(size_t role) const;
    bool occupied_cell(size_t slot) const;
    void assign(size_t role, size_t slot);
    void cancel_position();
    void begin_position(size_t role);
    void toggle_role();
    Result<bool> capture_equipment();
    void activate();
    void resolve_confirmation();
    void inspect_role();
    void navigate(bool right);
    void outline(std::vector<uint8_t>& image, int left, int top, int right, int bottom, unsigned color) const;

public:
    Deployment(const std::filesystem::path& root, Events events, Save::State raw, Disc disc);
    size_t capacity() const;
    bool done() const;
    bool positioning() const;
    bool facing() const;
    const std::vector<unsigned long long>& roles() const;
    const std::vector<int>& assignments() const;
    const s2ne::Events& events() const;
    const s2ne::Save::State& raw() const;
    const std::vector<s2ne::Events::DeploymentSlot>& slots() const;
    std::pair<int, int> cell_screen(int x, int y) const;
    size_t selected_role() const;
    size_t option() const;
    bool select_option(size_t option);
    bool select_role(size_t role);
    bool select_cell(Battle::Point cell);
    bool respond_confirmation(bool yes);
    bool confirmation_pending() const;
    void frame_presented();
    void scan(unsigned code);
    void choose_position();
    void mouse(int x, int y, bool left, bool right = false);
    Result<Save::State> prepare_commit() const;
    std::vector<uint8_t> wide_frame(const std::vector<uint8_t>& center);
    std::vector<uint8_t> draw(bool world_only = false, FullWidthCanvas* canvas = nullptr);
};
}    //namespace s2ne
