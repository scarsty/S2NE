#pragma once
#include "army_equipment_menu.h"
#include "battle_reports.h"
#include "combat.h"
#include "save_bridge.h"
#include "save_menu.h"
#include "save_store.h"
#include "shop_menu.h"
#include "town_data.h"
#include "worldmap.h"
#include <ctime>
#include <nlohmann/json.hpp>
namespace s2ne
{
class Inn
{
    Menu font_;
    Interbattle background_;
    ShopMenu graphics_;
    std::vector<uint8_t> towns_, rumor_;
    s2ne::Random random_{ 1 };
    unsigned selected_ = 1;
    bool closed_ = false, reading_ = false;
    std::string blocker_;
    std::optional<BattleReports> roster_;
    std::optional<ArmyEquipmentMenu> equipment_menu_;
    std::optional<Worldmap> world_;
    Disc domain_;
    std::optional<Settings> settings_;
    std::optional<Confirmation> auto_confirm_;
    bool equipment_changed_ = false;
    bool title_requested_ = false;
    bool presentation_driven_ = false;
    std::optional<Confirmation> title_confirm_;
    std::optional<SaveMenu> saves_;
    std::optional<Confirmation> overwrite_;
    std::optional<SaveBridge::Restored> loaded_;
    std::function<Result<Save::State>(unsigned, const Save::State&)> save_writer_;
    Settings::State settings_initial_;
    std::optional<Settings::State> settings_result_;
    std::filesystem::path root_;
    std::filesystem::path save_root_;

public:
    static constexpr std::array<unsigned, 10> labels{ 193, 194, 128, 129, 195, 192, 80, 131, 123, 190 };
    static constexpr std::array<Interbattle::Rect, 10> rectangles{ { { 473, 60, 562, 176 }, { 310, 106, 380, 216 }, { 534, 184, 617, 303 }, { 455, 179, 530, 299 }, { 292, 243, 412, 479 }, { 292, 206, 352, 240 }, { 398, 209, 420, 252 }, { 48, 167, 120, 302 }, { 250, 158, 294, 202 }, { 121, 151, 266, 444 } } };
    explicit Inn(const std::filesystem::path& root, Disc disc = Disc::one);
    static Result<std::vector<uint8_t>> try_load_towns(const std::filesystem::path& root);
    bool closed() const;
    bool select_auto_equipment();
    bool confirm_auto_equipment(Save::State& state);
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
    const Worldmap* worldmap() const;
    const std::vector<unsigned char>& rumor() const;
    const std::string& blocker() const;
    std::vector<uint8_t> town_rumor(const Save::State& state, unsigned branch) const;
    std::optional<Settings::State> take_settings();
    void prepare_settings(const Settings::State& state);
    bool settings_active() const;
    void settings_event(const Settings::Event& event);
    void set_save_writer(std::function<Result<Save::State>(unsigned, const Save::State&)> writer);
    std::optional<SaveBridge::Restored> take_loaded();
    bool saves_active() const;
    void save_event(const SaveMenu::Event& event, const Save::State& state);
    bool take_equipment_changed();
    void finish_auto(Save::State& state);
    void scan(unsigned code, Save::State& state);
    void mouse(int x, int y, bool left, bool right, Save::State& state);
    std::vector<uint8_t> draw(const Save::State& state, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
