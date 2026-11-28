#pragma once
#include "action_audio.h"
#include "ai.h"
#include "combat.h"
#include "combat_visual.h"
#include "disc.h"
#include "events.h"
#include "promotion_transaction.h"
#include "random_stream.h"
#include "skill94_transaction.h"
#include "skills.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <string>

namespace s2ne
{
// Original terrain/action-point arithmetic with basic contact melee; scored AI and abilities remain pending.
class Battle
{
    struct PhysicalExchangeDiagnostic;

public:
    struct AiSkill
    {
        unsigned id{};
        size_t target{};
        bool second{};
        s2ne::Score score{};
        int destination_record{};
        int center_x{}, center_y{};
        std::optional<unsigned> direction;
    };

private:
    std::shared_ptr<Skills> ai_skills_;
    std::shared_ptr<Skill94Transaction> ai_damage_;
    std::pair<int, int> ai_skill_camera_{};
    std::optional<AiSkill> ai_skill_;
    std::optional<size_t> ai_skill_actor_;
    bool ai_skill_waiting_ = false, ai_skill_failed_ = false, ai_skill_replan_ = false;

public:
    struct AiUnscoredSkill
    {
        unsigned actor{}, skill{}, effect{}, shape{};
    };
    std::vector<AiUnscoredSkill> ai_unscored_skills;
    void set_ai_skills(const Skills& rules);
    void set_ai_damage(const Skill94Transaction& rules);
    void set_ai_skill_camera(std::pair<int, int> camera);
    bool ai_skill_pending() const;
    const std::optional<AiSkill>& ai_skill_request() const;
    std::optional<size_t> ai_skill_actor() const;
    bool ai_skill_ready() const;
    void begin_ai_skill();
    void fail_ai_skill();
    bool ai_skill_failed() const;
    void finish_ai_skill();
    struct Cell
    {
        int elevation{}, ground{}, object{};
    };
    struct Point
    {
        int x{}, y{};
        bool operator==(const Point&) const = default;
    };

private:
    unsigned width_{}, height_{};
    std::vector<Cell> cells_;
    std::vector<unsigned> ground_attributes_, object_attributes_;
    std::function<unsigned(unsigned, unsigned)> effect_audio_;
    unsigned movement_sound_channel_{}, movement_sound_id_{};
    int movement_sound_frame_ = -1;
    int physical_sound_{};
    void movement_sound(const Events::Unit& pose);
    std::vector<Point> teleport_pads_;
    std::function<bool(unsigned, Point)> teleport_presenter_;
    size_t teleport_cursor_{};
    std::optional<Point> teleport_destination_;
    bool teleport_pending_{};
    bool teleport_movement_{};
    std::vector<uint8_t> movement_;
    std::vector<Point> sealed_cells_;
    std::array<int, 12> config_{};
    unsigned turn_ = 1;
    bool hostile_side_ = false;
    std::vector<uint8_t> effect_palette_;
    std::array<std::vector<uint8_t>, 3> movement_effects_;
    std::array<std::vector<uint8_t>, 2> selection_frames_;
    std::vector<uint8_t> ground_cursor_;
    struct ProjectileSprite
    {
        unsigned width{}, height{};
        std::vector<uint8_t> pixels;
    };
    std::array<ProjectileSprite, 16> projectile59_;
    std::array<ProjectileSprite, 5> finish_directions_;
    std::optional<Events::EffectFlight> projectile_;
    unsigned projectile_elapsed_{};

public:
    void set_effect_audio(std::function<unsigned(unsigned, unsigned)> callback);
    unsigned scripted_movement_sound_id(const Events::Unit& pose) const;
    unsigned scripted_effect_sound(unsigned id, unsigned channel) const;
    bool projectile_active() const;

private:
    std::array<std::vector<uint8_t>, 4> movement_mouse_;
    std::array<Point, 4> movement_mouse_sizes_{};
    Point mouse_position_{ -1, -1 }, hovered_cell_{ -1, -1 };
    Point hovered_{ -1, -1 };

public:
    // Raw inputs used by sub_476707; offsets refer to its runtime unit record.
    struct MovementContext
    {
        unsigned extra_flags{}, status34{}, status118{}, status116{};
        bool has_usable_ability{};    // sub_443235(side, unit, 3) == 1
        unsigned movement_class{}, sprite_height{};
    };

private:
    std::vector<MovementContext> movement_contexts_;
    std::vector<Events::Unit> movement_poses_;
    unsigned movement_elapsed_ = 0;
    unsigned movement_interval_ = 80;
    int moving_unit_ = -1;
    bool movement_animation_enabled_ = false;
    bool attack_animation_ = false;
    s2ne::Feedback combat_feedback_;
    struct RestFeedback
    {
        size_t target{};
        std::array<int, 3> amounts{};
        unsigned step{}, elapsed{};
        bool presented{};
    };
    std::optional<RestFeedback> rest_feedback_;
    bool curse33_feedback_{};
    bool skill94_feedback_{}, skill94_feedback_presented_{}, skill94_death_lock_{};
    unsigned skill_feedback_effect_{};
    int skill_feedback_caster_hp_{}, skill_feedback_caster_max_hp_{};
    bool skill_feedback_reverse_{};
    std::function<void()> skill94_feedback_commit_;
    std::vector<std::pair<size_t, bool>> status_feedback_targets_;
    size_t status_feedback_actor_{}, status_feedback_cursor_{};
    std::function<void()> status_feedback_commit_;
    std::optional<Events::Unit> reaction_pose_;
    std::function<void(std::vector<Events::Unit>&, size_t, size_t)> death_settlement_;

public:
    struct PhysicalSettlement
    {
        size_t actor{}, target{};
        unsigned actor_id{}, target_id{};
        int hp_lost{}, raw_xp{};
        bool killed{};
    };

private:
    std::function<void(std::vector<Events::Unit>&, const PhysicalSettlement&)> physical_settlement_;
    std::function<Result<bool>(const std::vector<Events::Unit>&, const std::vector<PhysicalSettlement>&)> physical_loot_commit_;
    std::function<Result<bool>(size_t, size_t)> physical_death_presenter_;
    std::function<void(bool)> physical_transaction_finish_;
    std::function<void(std::vector<PhysicalSettlement>, std::function<void(bool)>)> physical_growth_;
    std::function<Result<bool>(bool)> physical_loot_finish_;
    std::shared_ptr<Events> hostile_resources_;
    std::shared_ptr<PromotionTransaction> physical_equipment_;
    std::vector<int> ai_kind_classes_;
    std::vector<int> kind_movement_types_;
    std::vector<std::array<std::vector<uint8_t>, 30>> action_sequences_;
    std::array<int, 0x110> ai_config_{};
    size_t hostile_cursor_ = 0;
    int hostile_actor_ = -1, hostile_target_ = -1, hostile_attack_slot_ = -1;
    unsigned hostile_wait_ = 0;
    bool hostile_actions_active_ = false;
    bool controller_hostile_ = true;
    bool automatic_actions_completed_ = false;
    std::vector<Point> hostile_path_;
    size_t hostile_path_cursor_ = 0;
    bool hostile_retreat_ = false;
    bool hostile_special_ = false;

    bool original_rules_ = false;
    bool mask_surfaces_ = false;
    s2ne::Random random_{ 1 };
    std::optional<s2ne::Random> teleport_random_baseline_;
    std::optional<uint32_t> random_seconds_;
    int selected_ = -1;
    unsigned selected_context_ = 0;
    bool auto_selection_ = false;
    size_t next_player_unit_ = 0;

public:
    struct AiCandidateDiagnostic
    {
        unsigned actor{}, target{}, action{};
        int chance{}, damage{}, value{};
        int counter_action{ -1 }, counter_chance{}, counter_damage{}, counter_value{}, weighted_value{};
        int score{};
        bool rejected_counter{};
    };
    std::vector<AiCandidateDiagnostic> ai_candidate_diagnostics;
    struct PhysicalDiagnostic
    {
        size_t actor{}, target{};
        int legacy_chance{}, original_chance{}, accuracy_bonus{}, height{}, mode{};
    };
    std::optional<PhysicalDiagnostic> last_physical_diagnostic_;
    const std::optional<PhysicalDiagnostic>& physical_diagnostic() const;

public:
    enum class State
    {
        idle,
        command,
        movement,
        attack_menu,
        attack_target,
        finish_confirm,
        item_menu
    };
    static constexpr std::array<unsigned, 6> command_icons{ 158, 160, 162, 164, 188, 166 };
    // sub_43AE24 copies these six availability masks before packing the
    // surviving entries into the original 32-pixel command strip.
    static constexpr std::array<uint32_t, 6> command_masks{
        0x80000000u, 0x40000000u, 0x20000000u,
        0x10000000u, 0x00010000u, 0x08000000u
    };
    static constexpr std::array<unsigned, 8> attack_icons{ 168, 170, 172, 174, 176, 178, 180, 182 };
    static constexpr std::array<uint32_t, 8> attack_masks{
        0x04000000u, 0x02000000u, 0x01000000u, 0x00800000u,
        0x00400000u, 0x00200000u, 0x00100000u, 0x00080000u
    };

private:
    State state_ = State::idle;
    int command_ = 0, target_ = -1, attack_index_ = 0, requested_command_ = -1;
    int requested_detail_unit_ = -1;
    std::function<bool(const Events::Unit&, int)> command_request_available_;

public:
    int requested_command() const;
    int requested_detail_unit() const;
    void clear_requested_command();
    void commit_external_action();
    void commit_equipment_action(Events::Unit& actor);
    // Modal owners supply their proven complete skill/promotion eligibility.
    void set_command_request_availability(std::function<bool(const Events::Unit&, int)> available);

private:
    bool moved_ = false, acted_ = false;
    Events::Unit before_move_{};
    std::vector<int> distances_, previous_;
    std::vector<uint8_t> movement_categories_;
    std::vector<Point> path_;
    std::string message_ = "Select a unit to open its original command bar";
    uint32_t random_seconds() const;
    int percent();
    int roll(int n);
    void recover_rest(std::vector<Events::Unit>& units, size_t index);

public:
    Battle(unsigned w, unsigned h, std::vector<Cell> cells);
    static Result<Battle> try_load(const std::filesystem::path& root, unsigned map, Disc disc = Disc::one);
    static Battle load(const std::filesystem::path& root, unsigned map, Disc disc = Disc::one);
    bool valid(Point p) const;
    Cell cell(Point p) const;
    bool skill_area_cell(Point p) const;
    // 42CE30/42CE6D tests the transported appearance, not caster movement or route cost.
    bool spell_transfer_terrain(const Events::Unit& unit, Point p) const;
    std::vector<Point> spell_transfer_destinations(const Events::Unit& caster, const Events::Unit& transported, const std::vector<Events::Unit>& units) const;
    std::optional<Point> spell_summon_destination(const Events::Unit& caster, unsigned appearance,
        const std::vector<Events::Unit>& units) const;
    void clear_runtime_object(Point p);
    // 42ACF4 differs from storage bounds: border and raised top-edge cells are excluded.
    unsigned object_category(Point p) const;
    std::vector<Point> object_cells(unsigned category) const;
    // 455828 allied control mode0 excludes ordinary NPC selector1.
    void settle_object_flags(Events& events) const;
    const std::vector<Point>& teleport_pads() const;
    void set_teleport_presenter(std::function<bool(unsigned, Point)> presenter);
    bool teleport_pending() const;
    void rollback_teleport();
    Result<bool> try_finish_teleport();
    void finish_teleport();
    // 4608B2 case35 -> 42FFA2 -> 45DB10, once per unit at side end.
    unsigned settle_teleport_pads(std::vector<Events::Unit>& units, bool hostile);
    bool target_surface(Point p) const;
    struct TerrainEffect
    {
        int result{};
        std::array<uint8_t, 3> bytes{ 1, 0, 0 };
    };
    TerrainEffect terrain_effect(const Events& resources, const Events::Unit& unit, Point p) const;
    bool target_visible(const Events::Unit& target, std::span<const Events::Unit> opposing, unsigned opposing_mode) const;
    std::vector<Point> action_cells(Point origin, Events::ActionRange range) const;
    // 408455 compares the attacker's row parity, not the defender's.
    static unsigned attack_mode(Point attacker, Point defender, unsigned defender_direction, bool simple_rules = false);
    struct ActionTarget
    {
        size_t unit_index{};
        Point position;
        unsigned direction{};
    };
    std::vector<ActionTarget> action_targets(const Events& resources, const Events::Unit& actor,
        const Events::ActionRecord& action, std::span<const Events::Unit> candidates,
        std::span<const Events::Unit> opposing, unsigned opposing_mode) const;
    std::vector<unsigned> counter_actions(const Events& resources, const Events::Unit& defender,
        Point attacker_position, std::span<const Events::Unit> attackers, std::span<const Events::Unit> defenders,
        unsigned attacker_mode, uint8_t defender_39, uint32_t defender_status3c_dword) const;
    Result<std::vector<size_t>> try_physical_target_group(const Events& resources, const Events::Unit& actor, const Events::ActionRecord& action, const std::vector<Events::Unit>& units, size_t primary) const;
    struct PhysicalActionContext
    {
        size_t actor{}, target{};
        Events::Unit actor_snapshot, target_snapshot;
        Events::ActionRecord action{};
        std::optional<Events::ActionRecord> counter;
        int mode{}, counter_tactics{};
    };
    static uint8_t physical_status(const Events::Unit& unit, unsigned offset);
    static uint32_t physical_status_dword(const Events::Unit& unit);
    Result<std::vector<PhysicalActionContext>> try_physical_contexts(const Events& resources, const std::vector<Events::Unit>& units, size_t actor, size_t primary, const Events::ActionRecord& action, bool reuse_primary = false);
    struct AttackAvailability
    {
        uint32_t mask{};
        std::array<int, 8> actions{ -1, -1, -1, -1, -1, -1, -1, -1 };
    };
    AttackAvailability attack_availability(const Events& resources, const Events::Unit& actor,
        std::span<const Events::Unit> candidates, std::span<const Events::Unit> opposing, unsigned opposing_mode) const;
    static int original_distance(Point from, Point to);
    // sub_448CD6: four diagonal neighbours in the staggered grid.
    static bool adjacent(Point a, Point b);
    static Point center(Point p);
    Point pick(float x, float y, int camera_x, int camera_y) const;
    bool orient_finish_threat(const Events& resources, std::vector<Events::Unit>& units);
    void finish_action(std::vector<Events::Unit>& units, bool confirmed);
    Result<bool> try_restore_turn(unsigned value);
    void restore_turn(unsigned value);
    int selected() const;
    bool player_controlled(const Events::Unit& unit, size_t index) const;
    // sub_4372EF reads 4F7D04's high byte (SET field four), not field six.
    void set_auto_selection(bool enabled);
    bool player_selection_eligible(const Events::Unit& unit, unsigned context_flags = 0) const;
    bool select_unit(std::vector<Events::Unit>& units, size_t index, unsigned context_flags = 0);
    bool select_next_player_unit(std::vector<Events::Unit>& units, bool restart = false, unsigned context_flags = 0);
    bool select_or_inspect_unit(std::vector<Events::Unit>& units, size_t index, unsigned context_flags = 0);
    const std::string& message() const;
    State state() const;
    int command_index() const;
    int target() const;
    bool select_attack_target(const std::vector<Events::Unit>& units, size_t index);
    // This is the six-bit portion of dword_50049C consumed by sub_43AE24.
    // It deliberately exposes availability rather than inventing labels: the
    // renderer uses the same source icon IDs and marks unavailable slots.
    uint32_t command_availability(const std::vector<Events::Unit>& units) const;
    void refresh_command_availability(const std::vector<Events::Unit>& units);
    bool command_available(const std::vector<Events::Unit>& units, int index) const;
    int command_slot(const std::vector<Events::Unit>& units, int command) const;
    int command_from_slot(const std::vector<Events::Unit>& units, int slot) const;
    int command_count(const std::vector<Events::Unit>& units) const;
    void prepare_commands(const std::vector<Events::Unit>& units);
    const std::vector<int>& route_costs() const;
    const std::vector<Point>& path() const;
    // sub_477BEB compares signed remaining AP, not path length or terrain.
    static uint8_t movement_category(int remaining, unsigned flags, int ability_threshold, int attack_threshold);
    void set_movement_contexts(std::vector<MovementContext> contexts);
    const std::vector<uint8_t>& movement_categories() const;
    // Optional raw destination-record override, applied after build_routes.
    // Normal operation generates these values in build_routes itself.
    void set_movement_categories(const std::vector<uint8_t>& categories);
    const std::vector<s2ne::Battle::PhysicalExchangeDiagnostic>& physical_exchange_diagnostics() const;
    const s2ne::Feedback& combat_feedback() const;
    bool combat_feedback_active() const;
    void lock_skill94_death(bool locked);
    void feedback_presented();
    bool begin_skill94_feedback(size_t actor, size_t target, int hp, int damage, std::function<void()> commit, unsigned effect = 1, int caster_hp = 0, int caster_max_hp = 0, bool reverse = false, bool hit_flash = false);
    bool begin_healing_feedback(size_t actor, size_t target, int hp, int healing, std::function<void()> commit, bool sound = true);
    bool begin_curse33_feedback(size_t actor, size_t target, int hp, bool hit, std::function<void()> commit);
    bool begin_status_area_feedback(size_t actor, std::vector<std::pair<size_t, bool>> targets,
        const std::vector<Events::Unit>& units, std::function<void()> commit, bool blessing3 = false, std::vector<int> amounts = {}, bool healing = false);
    bool status_feedback_blessing3_{}, status_feedback_healing_{};
    std::vector<int> status_feedback_amounts_;
    bool next_status_feedback(const std::vector<Events::Unit>& units);
    template <class ScreenPosition, class Silhouette, class Text>
    void render_combat_feedback(const std::vector<Events::Unit>& units, ScreenPosition screen_position, Silhouette silhouette, Text text) const
    {
        if (rest_feedback_ && rest_feedback_->target < units.size())
        {
            const auto& rest = *rest_feedback_;
            const auto position = screen_position(units[rest.target]);
            const std::array<const char*, 3> labels{ "HP ", "MP ", "ST " };
            int x = position.x;
            int y = position.y - 64 - int(rest.step) * 4;
            bool first = true;
            for (size_t i = 0; i < rest.amounts.size(); ++i)
            {
                if (rest.amounts[i] <= 0) { continue; }
                auto label = labels[i] + std::to_string(rest.amounts[i]);
                if (first)
                {
                    x += 32 - int(label.size()) * 8;
                    first = false;
                }
                text(label, x, y, i == 0 ? 0x68u : 0x26u, 0u);
                y += 24;
            }
            return;
        }
        const auto& f = combat_feedback_;
        if (!f.active() || f.target >= units.size())
        {
            return;
        }
        auto unit = units[f.target];
        if (skill94_feedback_)
        {
            unit.hp = f.hp_after;
        }
        auto position = screen_position(unit);
        if (f.silhouette())
        {
            silhouette(unit, position, f.silhouette_palette());
        }
        if (f.text_visible())
        {
            text(f.text(), position.x + f.text_x(), position.y + f.text_y(), f.text_foreground(), f.text_background());
        }
    }
    void set_death_settlement(std::function<void(std::vector<Events::Unit>&, size_t, size_t)> callback);
    void set_physical_settlement(std::function<void(std::vector<Events::Unit>&, const PhysicalSettlement&)> callback);
    void set_physical_growth(std::function<void(std::vector<PhysicalSettlement>, std::function<void(bool)>)> callback);
    void set_physical_loot_finish(std::function<Result<bool>(bool)> callback);
    void set_physical_loot_commit(std::function<Result<bool>(const std::vector<Events::Unit>&, const std::vector<PhysicalSettlement>&)> callback);
    void set_physical_death_presenter(std::function<Result<bool>(size_t, size_t)> callback, std::function<void(bool)> finish);
    void resume_physical_death();

private:
    struct PhysicalExchangeDiagnostic
    {
        size_t actor{}, target{};
        int damage{};
        bool reverse{}, consecutive{};
    };
    std::vector<PhysicalExchangeDiagnostic> physical_exchange_diagnostics_;
    struct PhysicalExchangeStep
    {
        PhysicalActionContext context;
        int damage{};
        bool consecutive{}, reverse{};
    };
    struct PhysicalExchange
    {
        std::vector<PhysicalExchangeStep> forward, replies;
        std::vector<PhysicalSettlement> forward_awards, counter_awards;
        std::vector<Events::Unit> forward_targets, baseline_units;
        s2ne::Random baseline_random{ 1 };
        std::vector<std::pair<size_t, size_t>> deaths;
        size_t cursor{}, counter_cursor{};
        std::optional<std::pair<size_t, size_t>> death;
        bool death_started{};
        bool reverse{}, group_presented{}, group_status_applied{};
        bool growth_pending{};
    };
    std::optional<PhysicalExchange> physical_exchange_;
    std::optional<Events::ActionRecord> pending_action_status_;
    int apply_physical_action_status(std::vector<Events::Unit>& units, size_t b, const Events::ActionRecord& action, int result, std::optional<int> status_roll = std::nullopt);
    int apply_physical_status_tail(std::vector<Events::Unit>& units, size_t actor, size_t target, const Events::ActionRecord& action, int damage);
    void settle_physical(std::vector<Events::Unit>& units, size_t a, size_t b, int lost);

public:
    void tick_combat_feedback(std::vector<Events::Unit>& units, unsigned milliseconds);
    bool movement_presentation_driven_{}, movement_presented_{};
    void enable_movement_presentation();
    void movement_presented();
    void enable_movement_animation();
    bool animating_movement() const;
    bool attack_animation_active() const;
    void tick_movement(std::vector<Events::Unit>& units, unsigned milliseconds);
    std::vector<Events::Unit> render_units(const std::vector<Events::Unit>& units) const;
    void cancel();
    void cancel(std::vector<Events::Unit>& units);
    static Point surface_center(Point p, Cell c);
    Point screen_center(Point p, int cx, int cy) const;
    void set_mask_surfaces(bool enabled);
    Point pick_surface(float x, float y, int cx, int cy) const;
    Point command_origin(const std::vector<Events::Unit>& units, int cx, int cy) const;
    // 477DE4 rejects hostile occupants; 477E50 only clears the landing marker
    // for allied occupants. Friendly roles block destinations, not transit.
    bool player_route_passable(const std::vector<Events::Unit>& units, const Events::Unit& actor, Point p) const;
    void build_routes(const std::vector<Events::Unit>& units);
    std::vector<Point> occupied_route_destinations_;
    bool preview(Point p);
    int attack_menu_cost(const Events::Unit& unit) const;
    int attack_cost(const Events::Unit& unit) const;
    static int executable_attack_slot(const Events::ActionRecord& action);
    std::optional<Events::ActionRecord> executable_action(const std::vector<Events::Unit>& units, size_t a, size_t b, int slot = -1) const;
    bool can_attack(const std::vector<Events::Unit>& units, size_t a, size_t b, int slot = -1) const;
    bool attack_command_available(const std::vector<Events::Unit>& units, int index) const;
    int attack_index() const;
    int attack_count(const std::vector<Events::Unit>& units) const;
    int attack_from_slot(const std::vector<Events::Unit>& units, int slot) const;
    void cycle_command(int delta);
    bool command_available_for_cycle(int index) const;

private:
    uint32_t available_mask_{};
    unsigned attack_menu_mask_{};
    int item_choice_ = 0;

public:
    int item_choice() const;
    void cycle_target(const std::vector<Events::Unit>& units);
    bool activate(std::vector<Events::Unit>& units);
    void hover(const std::vector<Events::Unit>& units, Point p);
    int attack_target_at(const std::vector<Events::Unit>& units, Point p) const;
    bool mouse(std::vector<Events::Unit>& units, float x, float y, bool pressed, int cx, int cy, unsigned context_flags = 0);
    int occupant(const std::vector<Events::Unit>& units, Point p) const;
    std::vector<std::array<int, 5>> observed_movement_edges(const std::vector<Events::Unit>& units, size_t actor, bool occupied) const;
    int step_cost(const Events::Unit& u, Point from, Point to) const;
    bool can_step(const std::vector<Events::Unit>& units, size_t index, Point to) const;
    std::optional<std::vector<Point>> scripted_route(const Events::Unit& unit, Point destination) const;
    unsigned scripted_frame_count(const Events::Unit& unit, unsigned facing) const;
    int hovered_unit(const std::vector<Events::Unit>& units) const;
    Point hovered_cell() const;
    static unsigned facing(Point from, Point to, unsigned current);
    bool step(std::vector<Events::Unit>& units, size_t index, Point to, bool allied_transit = false);
    // sub_408CB7 / sub_40A622: equal-height base arithmetic only; class/status/skill modifiers remain pending.
    static int ordinary_damage(int attack, int defense, int divisor, int minimum_roll);
    void seed(unsigned value);
    // Callback must stage its own domain mutations; failure leaves the
    // combat stream untouched. Permutation initialization is never repeated.
    bool mutate_combat_random(const std::function<bool(s2ne::Random&, uint32_t)>& transaction);
    bool present_skill32_preparation(std::vector<Events::Unit>& units, size_t index, size_t target, bool second = false);
    bool mutate_random(const std::function<bool(RandomStream&, uint32_t)>& transaction);
    bool preview_action_weight(const Events& resources, const Events::Unit& actor,
        const Events::Unit& target, const Events::ActionRecord& action);
    std::array<uint8_t, 3> original_42c88f_bytes(const Events& resources, const Events::Unit& unit) const;
    unsigned turn() const;
    bool hostile_side() const;
    bool hostile_actions_active() const;
    int moving_unit() const;
    int moving_actor() const;
    void set_hostile_resources(const Events& resources);
    int physical_action_preview(const Events& resources, const Events::Unit& actor, const Events::Unit& defender, const Events::ActionRecord& action, bool counter = false, int counter_mode = 0) const;
    struct AiPlanDiagnostic
    {
        unsigned actor_id{};
        int behavior{}, appearance_kind{}, dispatch{};
        bool unsupported_goal{}, unsupported_special{}, no_attack_candidate{};
        int movement_mode{}, route_count{}, destination_record{};
        bool unsupported_health{};
        unsigned target_id{};
        int target_value{};
        bool target_hidden{};
    };
    std::optional<AiPlanDiagnostic> ai_plan_diagnostic_;
    const std::optional<AiPlanDiagnostic>& ai_plan_diagnostic() const;
    bool npc_controller_ = false;
    bool automatic_actions_completed() const;
    void begin_npc_actions(std::vector<Events::Unit>&);
    bool tick_npc_actions(std::vector<Events::Unit>& units, unsigned milliseconds);
    void begin_automatic_actions(std::vector<Events::Unit>& units);
    bool tick_automatic_actions(std::vector<Events::Unit>& units, unsigned milliseconds);
    void begin_hostile_actions(std::vector<Events::Unit>& units);

private:
    void begin_controller_actions(bool hostile);
    static void idle_pose(Events::Unit& unit);

public:
    // Incremental executor: one route edge is committed per visible pose interval.
    // Returns true only after the last actor has finished and side AP is reset.
    bool tick_hostile_actions(std::vector<Events::Unit>& units, unsigned milliseconds);
    bool tick_hostile(const Events& resources, std::vector<Events::Unit>& units, unsigned milliseconds);

private:
    int hostile_contact_target(const std::vector<Events::Unit>& units, size_t actor) const;
    bool advance_hostile(std::vector<Events::Unit>& units, size_t index, Point to);
    void plan_hostile(const std::vector<Events::Unit>& units, size_t index);

public:
    void begin_hostile_side(std::vector<Events::Unit>& units);
    void tick_action_statuses(std::vector<Events::Unit>& units);
    void reset_side(std::vector<Events::Unit>& units, bool hostile);
    // Legacy contact-only test interface; production uses tick_hostile_actions.
    void end_turn(std::vector<Events::Unit>& units);
    void present_attack(const std::vector<Events::Unit>& units, size_t index, const Events::ActionRecord* chosen = nullptr);
    bool attack(std::vector<Events::Unit>& units, size_t a, size_t b, int slot = -1);
    void physical_feedback(std::vector<Events::Unit>& units, size_t a, size_t b, int damage, bool miss = false);
    int draw_forward_damage(const std::vector<Events::Unit>& units, size_t a, size_t b, const Events::ActionRecord& action, int attack_style, int* reverse_tactics = nullptr);
    bool draw_physical_consecutive(const PhysicalActionContext& context, bool reverse);
    bool draw_physical_instant(const PhysicalActionContext& context);
    Result<std::vector<PhysicalExchangeStep>> try_forward_exchange(const Events& resources, const std::vector<Events::Unit>& units, size_t actor, size_t primary, const Events::ActionRecord& action);
    int draw_reverse_damage(const PhysicalActionContext& context);
    Result<std::vector<PhysicalExchangeStep>> try_counter_exchange(const std::vector<Events::Unit>& units, const PhysicalActionContext& original);
    static void advance_physical_proficiency(Events::Unit& unit, const Events::ActionRecord& action);
    void advance_physical_exchange(std::vector<Events::Unit>& units);
    void rollback_physical_exchange(std::vector<Events::Unit>& units);
    bool strike(std::vector<Events::Unit>& units, size_t a, size_t b, int slot = -1);
    bool click(std::vector<Events::Unit>& units, Point p);
    bool has_movement_mouse() const;
    bool draw_movement_mouse(std::vector<uint8_t>& image) const;
    void draw_finish_directions(std::vector<uint8_t>& image, int left, int top, unsigned direction = 4) const;
    void draw_projectile(std::vector<uint8_t>& image, int cx, int cy) const;
    void draw_skill_cells(std::vector<uint8_t>& image, int cx, int cy, unsigned category, const std::function<bool(Point)>& includes, const std::function<void(std::vector<uint8_t>&, Point)>& mask = {}) const;
    void draw_selection(std::vector<uint8_t>& image, const std::vector<Events::Unit>& units, int cx, int cy, int layer = -1, const std::function<void(std::vector<uint8_t>&, Point)>& mask = {}) const;
};
}    //namespace s2ne
