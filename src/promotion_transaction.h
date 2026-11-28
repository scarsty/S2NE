#pragma once
#include "promotion.h"
#include <algorithm>

namespace s2ne
{
class PromotionTransaction
{
public:
    using Unit = Events::Unit;
    struct State
    {
        Unit unit;
        std::vector<uint8_t> inventory;
        RandomStream random;
    };
    struct Context
    {
        int game_mode = 0;
        // Must prepare class sprite/action caches and army recalculation against
        // the candidate only. Publishing live caches/save is the caller's swap.
        std::function<bool(State&)> rebuild;
        std::function<bool(State&)> recalculate_army;
    };
    static int word(int value);
    // 45C3E0: WORD arithmetic precedes clamps; current/base values are independent.
    static void apply_stat_increments(Unit& u, std::array<int, 9> growth);

private:
    Promotion promotion_;
    SharedBytes kinds_, items_, bin_, pal_;
    size_t records_{}, actions_{}, groups_{};
    unsigned item_count_{}, group_count_{};
    static unsigned w(const std::vector<uint8_t>& b, size_t p);
    static unsigned dw(const std::vector<uint8_t>& b, size_t p);
    static int sb(const std::vector<uint8_t>& b, size_t p);
    static int sw(const std::vector<uint8_t>& b, size_t p);
    size_t kind(unsigned id) const;
    s2ne::Result<size_t> try_item(unsigned id) const;
    size_t item(unsigned id) const;
    bool special(const Unit& u, int value) const;
    bool usable(const Unit& u, unsigned id, int category) const;
    bool weapon(const Unit& u, unsigned id) const;

public:
    PromotionTransaction() = default;
    static Result<PromotionTransaction> create(const std::filesystem::path& root);
    explicit PromotionTransaction(const std::filesystem::path& root);
    unsigned item_count() const;
    int item_defense(unsigned id) const;
    bool usable_item(const Unit& u, unsigned id, int category = 0) const;
    std::vector<uint8_t> item_name(unsigned id) const;
    bool learning_item(unsigned id) const;
    // 451DDA: inclusive one-based ordinal range; already learned entries skip the floor.
    bool learn_range(Unit& u, unsigned group, unsigned first, unsigned last) const;
    // 450FE1 effect65 teaches one ordinal; 451032 effect66 teaches 1..ordinal.
    bool use_learning_item(State& state, unsigned id, int category = 0) const;
    // 441143 reads runtime WORD +8 for the ability20/40 gate. Stage behavior
    // is not an equivalent substitute; supply the original role words in order.
    Result<uint8_t> try_aggregate_army_metadata(uint8_t existing, std::span<const Unit> units,
        std::span<const int> roles8, int mode = 0, int game_mode = 0) const;
    uint8_t aggregate_army_metadata(uint8_t existing, std::span<const Unit> units, std::span<const int> roles, int mode = 0, int game_mode = 0) const;
    std::optional<size_t> first_consumable_special(const Events::Unit& unit, unsigned special) const;
    bool remove_equipment(State& state, size_t slot, bool consume) const;
    bool equip(State& state, unsigned id) const;
    bool auto_equip(State& state, int category) const;
    // 45C9B1 uses one-based group and action ordinal, unlike the table APIs.
    static bool conditional_learning_allowed(const Unit& u, unsigned group, unsigned ordinal);
    bool learn_group(Unit& u, unsigned group, bool conditional = false) const;
    std::vector<uint8_t> newly_learned_name(const Unit& before, const Unit& after, unsigned group) const;
    void learn_relations(Unit& u) const;
    // 4659BD: index is the original WORD +D7 array index, not group ID.
    // 0..5 weapon proficiency, 6..16 learned-group proficiency +E3.
    bool award_proficiency(Unit& u, unsigned index, unsigned increment) const;
    void train(Unit& u, int group) const;
    std::optional<State> prepare(const State& before, const Promotion::Request& request, const Context& context,
        const RandomStream::TimeSource& time = {}) const;
};
}    //namespace s2ne
