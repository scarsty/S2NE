#pragma once

#include "runtime_error.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace s2ne
{

// Inputs retain the runtime names/meanings visible in opening.idb.asm.  No
// default RPG values are supplied; callers must provide decoded records.
// 47FD8E CRT state; unsigned overflow is part of the original recurrence.
class Random
{
public:
    explicit Random(uint32_t seed);
    void seed(uint32_t value);
    unsigned raw();
    int bounded(int bound, uint32_t seconds);
    void initialize(uint32_t seconds);
    template <class Clock>
    void initialize_with_clock(Clock clock)
    {
        unsigned value = 0;
        for (size_t column = 0; column < 10; ++column)
        {
            for (size_t row = 0; row < 10; ++row)
            {
                table_[row * 10 + column] = uint8_t(value++);
            }
        }
        auto draw = [&]()
        {
            if (++draws_ >= 200)
            {
                seed(clock());
                draws_ = 0;
            }
            return raw() % 100;
        };
        for (unsigned i = 0; i < 600; ++i)
        {
            auto a = draw(), b = draw();
            std::swap(table_[a], table_[b]);
        }
    }
    int percent(uint32_t seconds);
    unsigned draw_count() const;
    uint32_t seed_state() const;
    void restore_stream(uint32_t seed, unsigned calls);
    const std::array<uint8_t, 100>& table() const;

private:
    void before_draw(uint32_t seconds);
    uint32_t state_{};
    unsigned draws_{};
    std::array<uint8_t, 100> table_{};
};

// 45CFBB: value18/14/10/C are locals selected by input mode, not XP.
// Runtime names identify the signed word at the exact record offset.
struct CombatValueInput
{
    int value18{}, value14{}, value10{}, valueC{}, runtime134{}, runtime136{}, runtime138{}, runtimeC1{}, runtimeBF{}, runtimeC7{};
    int learned_first{}, learned_second{}, kind_multiplier{};
    uint32_t abilities{};
    bool equipment_18{};
    int configD6{}, configD7{}, configD8{}, configD9{}, configDA{}, configDB{}, configDC{}, configDF{};
};
Result<CombatValueInput> try_decode_combat_value(std::span<const uint8_t> unit, std::span<const uint8_t> config,
    std::span<const uint8_t> kind, unsigned first_group_end, int mode, bool equipment18);
Result<std::array<int, 4>> try_combat_value_parts(const CombatValueInput& in);
Result<int> try_combat_value(const CombatValueInput& in);

CombatValueInput decode_combat_value(std::span<const uint8_t> unit, std::span<const uint8_t> config, std::span<const uint8_t> kind, unsigned first_group_end, int mode, bool equipment18);
std::array<int, 4> combat_value_parts(const CombatValueInput& in);
int combat_value(const CombatValueInput& in);

struct ActionStatusInput
{
    int result{}, effect{}, duration{}, chance{}, roll{}, target_kind{}, target_kind17{};
    int target_runtimeB7{}, target_runtime124{}, target_runtime134{};
    bool target_status33_20{}, target_status3e_10{}, target_116_40{}, target_116_10{}, target_equipment14{};
};
struct ActionStatusResult
{
    unsigned function{}, mask{};
    int amount{}, duration{}, result{};
    bool redraw{};
};
// The action-table status tail of 43D93B; equipment/ability procs occur earlier.
ActionStatusResult action_status(const ActionStatusInput& in);

// 442C88/442CD5 retain the first-set-bit lookup and signed duration sentinel.
Result<bool> try_apply_timed_status(std::span<uint8_t> unit, uint32_t mask, int duration);
bool apply_timed_status(std::span<uint8_t> unit, uint32_t mask, int duration);

// 442BE6/442E96: modifiers apply only on insertion, not duration refresh.
Result<bool> try_apply_stat_status(std::span<uint8_t> unit, uint32_t mask, int duration, int delta134, int delta136);
bool apply_stat_status(std::span<uint8_t> unit, uint32_t mask, int duration, int delta134, int delta136);

// 443093 fixes mask 2 / slot 1, unlike the paired-stat function above.
Result<bool> try_apply_stat134_status(std::span<uint8_t> unit, int amount, int duration);
bool apply_stat134_status(std::span<uint8_t> unit, int amount, int duration);

// 442828 restores baseline words, then accumulates only active stored modifiers.
Result<bool> try_recalculate_status_stats(std::span<uint8_t> unit);
void recalculate_status_stats(std::span<uint8_t> unit);

// Executes the recovered action-table tail against the original runtime bytes.
Result<ActionStatusResult> try_apply_action_status(std::span<uint8_t> unit, ActionStatusInput input);
ActionStatusResult apply_action_status(std::span<uint8_t> unit, ActionStatusInput input);

// 442DC5 removes one entry, preferring its original stat-mask table.
Result<uint32_t> try_remove_one_status(std::span<uint8_t> unit);
uint32_t remove_one_status(std::span<uint8_t> unit);

// 46044D status-countdown prefix only; regeneration and damage follow it.
Result<bool> try_tick_status_durations(std::span<uint8_t> unit);
void tick_status_durations(std::span<uint8_t> unit);

// 45D52F: dword loads followed by SAR16 read the high signed word.
Result<bool> try_adjust_runtime_stats(std::span<uint8_t> unit, int percentage);
void adjust_runtime_stats(std::span<uint8_t> unit, int percentage);

// 40DDF0: a zero step exits before the difficulty term is added.
int stage_stat_adjustment(int average_value, unsigned baseline, unsigned divisor, int difficulty);

struct ExperienceInput
{
    int source_value{}, target_value{}, result{}, configE5{}, configE1{}, configE2{};
    bool target_status33_20{}, target_alive{};
};
// 409454/40948C swap context +8/+C before entering 4094C4.
Result<int> try_action_experience(const ExperienceInput& in);
int action_experience(const ExperienceInput& in);

struct AttackInput
{
    int attacker_attack{};     // runtime unit +134h
    int defender_defense{};    // runtime unit +136h
    int divisor{};             // sub_408CB7 damage divisor input
    int minimum_roll{};        // sub_408CB7 minimum-damage random result
};

struct DamageResult
{
    int damage{};
    int attack_minus_defense{};
};

// 40A622+CD..+172 compresses damage after class/action modifiers; 408CB7
// applies the minimum roll later. This helper retains only the equal-height
// unmodified subset used by Battle, not the complete original damage path.
Result<int> try_ordinary_damage(int attacker_attack, int defender_defense,
    int divisor, int minimum_roll);
int ordinary_damage(int attacker_attack, int defender_defense,
    int divisor, int minimum_roll);

struct DamageModifierInput
{
    int damage{}, class_matchup{}, attack_mode_bonus{}, action_coefficient{}, combat_value{};
    uint8_t defender_116{}, defender_3f{}, defender_3c{}, defender_39{}, attacker_3e{};
    bool defender_equipment_13{};
};
// 40A622: callers must decode the class matrix and action record first.
int modify_damage(const DamageModifierInput& in);

int damage_divisor(int percent_roll, int decimal_roll);
struct PhysicalDamageInput
{
    int attack{}, defense{}, height_difference{}, attack_mode{}, attacker_terrain_bonus{}, defender_terrain_bonus{}, context_bonus{}, divisor{};
    int random_multiplier{}, difficulty_bonus{}, minimum_roll{};
    uint8_t attacker_3c{}, attacker_118{}, defender_kind{};
    bool multiplayer{}, player_side{};
    DamageModifierInput modifiers;
};
Result<int> try_physical_damage(const PhysicalDamageInput& in);
int physical_damage(const PhysicalDamageInput& in);

// 407B83/407C35 with EBX=0 use signed configuration bytes and IDIV by two.
int preview_multiplier(int base, int spread);

struct PhysicalPreviewInput
{
    int attack{}, defense{}, height_difference{}, mode_bonus{}, attacker_terrain_bonus{}, defender_terrain_bonus{}, context_bonus{};
    int multiplier{}, difficulty_bonus{};
    bool multiplayer{}, player_side{}, immune{}, counter{};
    bool counter_fixed_damage{}, counter_equipment_30{};
    int counter_fixed_value{};
    uint8_t attacker_3c{}, attacker_118{}, defender_kind{};
    DamageModifierInput modifiers;
};
// 409BBA uses a deterministic divisor and a separate reverse-action coefficient.
int physical_preview(const PhysicalPreviewInput& in);

// 409BBA+25F: +30h/+34h are lost-HP percentages, not terrain values.
Result<int> try_lost_health_percent(int maximum_hp, int hp);
int lost_health_percent(int maximum_hp, int hp);

// 40A42D uses SAR (round down) in the >90 branch, not signed division.
int adjusted_tactics(int value, int condition, int percent_roll);
struct HitModifierInput
{
    int chance{}, action_type{}, action_bonus{}, weapon_accuracy{ 90 }, defender_equipment_penalty{};
    uint8_t defender_115{}, defender_3c{};
    bool defender_equipment_16{};
};
int modify_hit_chance(const HitModifierInput& in);

struct CounterDamageInput
{
    PhysicalDamageInput physical;
    int reduction_roll{}, fixed_value{};
    int minimum_roll{ 99 };
    bool fixed_damage{}, equipment_30{};
    bool* minimum_required{};
};
// 409060: the reverse attack has its own random reduction, before height/fixed damage.
Result<int> try_counter_damage(const CounterDamageInput& in);
int counter_damage(const CounterDamageInput& in);

struct HitChanceInput
{
    int attacker_tactics{}, defender_evasion{}, height_difference{}, attack_mode{}, context_bonus{};
    int attacker_condition{}, condition_roll{}, random_multiplier{}, difficulty_bonus{};
    bool multiplayer{}, player_side{};
    uint8_t defender_39{}, defender_3d{};
    uint32_t defender_status3c_dword{};
    bool preview{}, counter_preview{}, counter_accuracy_penalty{}, preview_immune{}, counter{};
    int counter_reduction_roll{};
    HitModifierInput modifiers;
};
// 408CB7 -> 40A42D -> 40A4B3 -> 407B83 -> 4080E7, before the percent draw.
int hit_chance(const HitChanceInput& in);
bool hit_succeeds(const HitChanceInput& in, int percent_roll);

// 4097EB returns a percentage threshold, not a hit count or AP cost.
struct ConsecutiveInput
{
    int attacker_tactics{}, defender_evasion{}, mode_adjustment{};
    int attacker_equipment_bonus{}, defender_equipment_penalty{};
    uint8_t attacker_115{}, defender_115{}, defender_3c{}, defender_3f{};
    bool has_attack_context{ true }, defender_equipment_13{}, defender_equipment_16{};
    unsigned opposing_mode{};
};
int consecutive_chance(const ConsecutiveInput& in);
int consecutive_mode_adjustment(int attack_mode, bool reverse_direction = false);
bool consecutive_succeeds(const ConsecutiveInput& in, int percent_roll);

// 408D04 calls 429F34 for both occupied cells: these are terrain heights,
// not random draws. The simple-rules branch at 408CE8 forces both to zero.
int terrain_height_difference(unsigned attacker_height, unsigned defender_height, bool simple_rules = false);
struct RandomDraws
{
    int decimal_random_0_9{};
    int percent_random_0_99{};
};
bool percent_at_most(const RandomDraws& r, int threshold);

// Only the contact/range facts proven in the disassembly are represented.
// Ranged geometry requires the decoded +23h/+27h/+28h record fields; this
// function deliberately refuses to infer a range from generic RPG rules.
struct RangeInput
{
    int distance{};
    int range{};
};
bool within_decoded_range(const RangeInput& in);

}    // namespace s2ne
