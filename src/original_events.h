#pragma once
#include "original_combat.h"
#include "original_stage_route.h"
#include "resource_result.h"
#include "text_encoding.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace s2ne
{
class OriginalEvents
{
    std::filesystem::path root_;
    SharedBytes bin_, pal_, kinds_, items_;
    std::vector<std::optional<uint8_t>> event_types_;
    std::vector<unsigned> json_event_turns_, json_event_flags_, json_event_conditions_, json_event_completion_flags_;
    std::vector<std::array<int, 5>> json_event_operands_;
    size_t events_{};
    int proficiency_difficulty_scale_{};
    size_t combat_config_{};
    unsigned word(size_t p) const;
    unsigned dword(size_t p) const;
    size_t entry(unsigned id) const;
    Result<size_t> try_record(size_t event) const;
    size_t record(size_t event) const;

public:
    struct Line
    {
        int speaker;
        std::vector<uint8_t> name, text;
        int music_before{}, music_after{};
    };
    struct JsonEventOverride
    {
        std::optional<unsigned> type, condition, flag;
    };
    std::vector<std::vector<Line>> json_line_overrides_;
    bool json_stage_loaded_{};
    std::vector<Line> opening_dialogue_override_, opening_choice_override_;
    std::vector<JsonEventOverride> json_event_overrides_;
    bool has_opening_dialogue_override(size_t event) const;
    unsigned stage{}, first_stage{}, alternate_stage{};
    unsigned fantasy_stage{};
    unsigned interbattle_town_id{};
    unsigned stage_music{};
    std::optional<int32_t> script_route;
    std::optional<int32_t> camera_step_delay;
    std::optional<std::array<int32_t, 2>> campaign_currencies;
    std::optional<std::array<int32_t, 5>> ending_statistics;
    struct Unit
    {
        unsigned id, appearance;
        int x, y;
        std::vector<uint8_t> name;
        int first_turn{}, last_turn{};
        int hp{}, max_hp{}, mp{}, st{}, sp{}, strikeback{}, attack{}, defense{}, fencing{}, tactics{}, behavior{};
        int retreat_role14{};
        int action_points{}, base_action_points{}, carry_points{}, movement_type{};
        int base_tactics{};                 // Signed runtime +C9h; tactics is the distinct current +138h.
        unsigned experience_threshold{};    // WORD runtime +111h, KINDSP record +0Eh.
        uint32_t runtime_flags30{};         // DWORD +30h; high byte overlaps status_flags (+33h).
        unsigned direction{}, animation_action{};
        uint8_t action_state{};    // Original runtime +2Dh: 0 idle, 1 selected, 2 finished, 3 moving, 4 moved.
        bool movement_pose{};
        unsigned movement_visual_class{};
        bool movement_camera_pose{};
        unsigned movement_frame{};
        int level{ 1 }, max_mp{}, max_st{}, max_sp{}, max_action_points{}, attack_limit{}, defense_limit{};    // max_action_points is WORD +12Ch; display uses WORD +12Eh / +0BFh.
        unsigned portrait{}, status_flags{ 0x80 }, experience{};
        std::vector<uint8_t> profession;
        std::array<unsigned, 6> equipment{};
        std::array<int, 4> equipment_stat_deltas{};    // runtime +13Ah/+13Ch/+13Eh/+140h
        std::array<unsigned, 4> initial_status{};
        std::array<unsigned, 11> learned{};
        std::array<unsigned, 11> learned_proficiency{};    // runtime +E3h; learned bits remain at +FBh
        std::array<unsigned, 6> weapon_proficiency{};      // runtime +D7h..+E1h
        unsigned special_proficiency{};                    // runtime +F9h
        uint32_t abilities{};                              // runtime +115h; +116h = bits 8..15, +118h = bits 24..31
        uint8_t status3d{}, status3e{}, status3f{};
        std::optional<uint8_t> status39;
        uint8_t status34{}, status3c{};    // runtime state, not fields in the 101-byte stage record
        std::optional<std::array<uint8_t, 0x13A>> action_status_runtime;
        // 468BAC writes the transient effect state at runtime +5F8, outside the packed +13A unit block.
        // It is intentionally not serialized as unit data; save round-trips preserve only durable fields.
        std::optional<int32_t> effect_state_5f8;
        unsigned kind_attribute{}, allegiance_tag{};
        unsigned runtime_role_selector{};
        int persistent_role6{};
        int ai_role10{ -1 };    // WORD +0Ah: [runtime+8] sar16, distinct from control WORD +8.
        unsigned ending_kills{}, ending_losses{};
        unsigned physical_kill_xp{};    // WORD runtime +119h, original stage record +53.
        std::optional<int> ending_combat_value;
        std::array<int, 40> relationships{};
        // sub_4274B5 partitions stage units at the first behavior 2/3/8; behavior 1 is allied AI.
        uint32_t flags30() const;
        void set_flags30(uint32_t value);
        bool hostile() const;
        bool controllable() const;
        bool npc_class() const;
        bool present(unsigned turn = 1) const;
    };
    unsigned map_id{};
    std::vector<std::array<unsigned, 4>> dynamic_object_chains;
    std::vector<Unit> units;
    std::optional<std::array<std::array<uint8_t, 33>, 2>> army_status;
    void release_summon_slot(const Unit& unit);
    std::optional<std::array<uint32_t, 2>> army_modes;    // Army runtime +DE, independent of summoned slots at +286.
    std::vector<uint8_t> briefing, stage_name, victory_objective, defeat_objective;
    std::array<int, 12> battle_config{};
    unsigned learned_ability_count{}, first_ability_count{}, second_ability_count{};
    int victory_relationship_increment() const;
    struct DeploymentSlot
    {
        int x{}, y{};
        unsigned direction{};
        int role{};
        bool fixed{};
    };
    std::vector<DeploymentSlot> json_deployment_slots_;
    bool json_deployment_loaded_{};
    std::vector<DeploymentSlot> deployment_slots() const;
    bool has_learned_ability(const Unit& unit, unsigned mode = 3, bool suppress_first = false, bool suppress_second = false) const;
    bool equipment_special(const Unit& unit, unsigned special) const;
    int action_status_kind(const Unit& unit) const;
    struct ActionRecord
    {
        unsigned index;
        std::array<int, 23> fields;
    };
    static s2ne::ActionStatusInput action_status_input(const ActionRecord& action, s2ne::ActionStatusInput target);
    static s2ne::ActionStatusResult apply_action_record_status(std::span<uint8_t> runtime, const ActionRecord& action, s2ne::ActionStatusInput target);
    Result<std::vector<ActionRecord>> try_action_records(const Unit& unit) const;
    std::vector<ActionRecord> action_records(const Unit& unit) const;
    bool action_equipment_allowed(const Unit& unit, const ActionRecord& action) const;
    void expire_army_reveal();
    void rebuild_army_modes(unsigned mode = 1);
    int stage_adjustment{};
    Result<bool> try_adjust_battle_entry_stats();
    void adjust_battle_entry_stats();
    Result<int> try_combat_value(const Unit& unit, int mode = 0, unsigned component = 0) const;
    int combat_value(const Unit& unit, int mode = 0, unsigned component = 0) const;
    struct ActionRange
    {
        int minimum{}, maximum{}, shape{};
    };
    ActionRange action_range(const Unit& unit, const ActionRecord& action) const;
    int maximum_action_range(const Unit& unit) const;
    unsigned action_proficiency(const Unit& unit, const ActionRecord& action) const;
    int action_accuracy_bonus(const Unit& unit, const ActionRecord& action) const;
    int action_effect_code(const Unit& unit, const ActionRecord& action) const;
    struct ActionEffect
    {
        unsigned resource{}, mode{};
        int sound{};
    };
    static ActionEffect action_effect(const ActionRecord& action, int code);
    // 43E36B chooses original CEL orientation by integer slope, not atan2.
    static unsigned effect_initial_direction(int source_x, int source_y, int target_x, int target_y, unsigned mode);
    struct EffectFlight
    {
        int source_x{}, source_y{}, target_x{}, target_y{}, x{}, y{}, slope{};
        unsigned axis{}, direction{};
    };
    static EffectFlight effect_flight(int source_x, int source_y, int target_x, int target_y, unsigned mode);
    static bool advance_effect_flight(EffectFlight& f);
    Result<std::array<unsigned, 2>> try_skill_class_masks(const Unit& unit) const;
    std::array<unsigned, 2> skill_class_masks(const Unit& unit) const;
    int equipment_evasion(const Unit& unit) const;
    int equipment_accuracy(const Unit& unit) const;
    Result<int> try_class_matchup(const Unit& attacker, const Unit& defender) const;
    int class_matchup(const Unit& attacker, const Unit& defender) const;
    unsigned movement_class(const Unit& unit) const;
    unsigned skill_target_class(const Unit& unit, unsigned status3c) const;
    unsigned difficulty{};
    std::array<uint8_t, 80> flags{};
    OriginalEvents() = default;
    static Result<OriginalEvents> create(const std::filesystem::path& root, unsigned level);
    explicit OriginalEvents(const std::filesystem::path& root, unsigned level);
    Result<bool> try_load_stage(unsigned id);
    void load_stage(unsigned id);
    void load_stage_validated(unsigned id);
    Result<bool> try_restore_unit_resources(Unit& unit) const;
    void restore_unit_resources(Unit& unit) const;
    size_t event_count() const;
    std::vector<uint16_t> save_event_states() const;
    Result<bool> try_restore_event_states(std::span<const uint16_t> states);
    void restore_event_states(std::span<const uint16_t> states);
    Result<int> try_event_word(size_t event, unsigned offset) const;
    int event_word(size_t event, unsigned offset) const;
    void clear_stage_flags();
    unsigned completion_flag(size_t event) const;
    unsigned type(size_t event) const;
    unsigned turn(size_t event) const;
    unsigned flag(size_t event) const;
    unsigned condition(size_t event) const;
    bool flag_trigger(size_t event) const;
    Result<bool> try_choose(size_t event, unsigned selected);
    void choose(size_t event, unsigned selected);
    bool branch_trigger(size_t event) const;
    void consume_branch(size_t event);
    bool flag_dialogue_trigger(size_t event) const;
    unsigned line_count(size_t event) const;
    bool turn_trigger(size_t event, unsigned current_turn) const;
    Result<Line> try_line(size_t event, size_t index) const;
    Result<bool> try_load_stage_json(unsigned id);
    Line line(size_t event, size_t index) const;
    Result<bool> try_load_opening_dialogue_override(const std::filesystem::path& path);
    void complete(size_t event);
};
}    //namespace s2ne
