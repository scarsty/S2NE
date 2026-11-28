#pragma once
#include "original_army_equipment_menu.h"
#include "original_battle_reports.h"
#include "original_combat.h"
#include "original_save_bridge.h"
#include "original_save_menu.h"
#include "original_save_store.h"
#include "original_shop_menu.h"
#include "original_town_data.h"
#include "original_worldmap.h"
#include <ctime>
#include <nlohmann/json.hpp>
namespace s2ne
{
class OriginalInn
{
    OriginalMenu font_;
    OriginalInterbattle background_;
    OriginalShopMenu graphics_;
    std::vector<uint8_t> towns_, rumor_;
    s2ne::Random random_{ 1 };
    unsigned selected_ = 1;
    bool closed_ = false, reading_ = false;
    std::string blocker_;
    std::optional<OriginalBattleReports> roster_;
    std::optional<OriginalArmyEquipmentMenu> equipment_menu_;
    std::optional<OriginalWorldmap> world_;
    OriginalDisc domain_;
    std::optional<OriginalSettings> settings_;
    std::optional<OriginalConfirmation> auto_confirm_;
    bool equipment_changed_ = false;
    bool title_requested_ = false;
    bool presentation_driven_ = false;
    std::optional<OriginalConfirmation> title_confirm_;
    std::optional<OriginalSaveMenu> saves_;
    std::optional<OriginalConfirmation> overwrite_;
    std::optional<OriginalSaveBridge::Restored> loaded_;
    std::function<Result<OriginalSave::State>(unsigned, const OriginalSave::State&)> save_writer_;
    OriginalSettings::State settings_initial_;
    std::optional<OriginalSettings::State> settings_result_;
    std::filesystem::path root_;
    std::filesystem::path save_root_;

public:
    static constexpr std::array<unsigned, 10> labels{ 193, 194, 128, 129, 195, 192, 80, 131, 123, 190 };
    static constexpr std::array<OriginalInterbattle::Rect, 10> rectangles{ { { 473, 60, 562, 176 }, { 310, 106, 380, 216 }, { 534, 184, 617, 303 }, { 455, 179, 530, 299 }, { 292, 243, 412, 479 }, { 292, 206, 352, 240 }, { 398, 209, 420, 252 }, { 48, 167, 120, 302 }, { 250, 158, 294, 202 }, { 121, 151, 266, 444 } } };
    explicit OriginalInn(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one);
    static Result<std::vector<uint8_t>> try_load_towns(const std::filesystem::path& root);
    bool closed() const;
    bool select_auto_equipment();
    bool confirm_auto_equipment(OriginalSave::State& state);
    bool leave_root();
    void set_save_root(const std::filesystem::path& root);
    bool title_requested() const;
    bool title_confirmation_active() const;
    void finish_title_confirmation();
    bool reading() const;
    bool roster_active() const;
    bool equipment_active() const;
    bool transitioning() const;
    bool world_active() const;
    void tick(unsigned clock);
    void enable_world_presentation();
    void enable_presentation_timing();
    void frame_presented();
    const OriginalWorldmap* worldmap() const;
    const std::vector<unsigned char>& rumor() const;
    const std::string& blocker() const;
    std::vector<uint8_t> town_rumor(const OriginalSave::State& state, unsigned branch) const;
    std::optional<OriginalSettings::State> take_settings();
    void prepare_settings(const OriginalSettings::State& state);
    bool settings_active() const;
    void settings_event(const OriginalSettings::Event& event);
    void set_save_writer(std::function<Result<OriginalSave::State>(unsigned, const OriginalSave::State&)> writer);
    std::optional<OriginalSaveBridge::Restored> take_loaded();
    bool saves_active() const;
    void save_event(const OriginalSaveMenu::Event& event, const OriginalSave::State& state);
    bool take_equipment_changed();
    void finish_auto(OriginalSave::State& state);
    void scan(unsigned code, OriginalSave::State& state);
    void mouse(int x, int y, bool left, bool right, OriginalSave::State& state);
    std::vector<uint8_t> draw(const OriginalSave::State& state, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
