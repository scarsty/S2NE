#pragma once
#include "original_promotion.hpp"
#include <algorithm>

namespace s2ne
{
class OriginalPromotionTransaction
{
public:
    using Unit = OriginalEvents::Unit;
    struct State
    {
        Unit unit;
        std::vector<uint8_t> inventory;
        OriginalRandom random;
    };
    struct Context
    {
        int game_mode = 0;
        // Must prepare class sprite/action caches and army recalculation against
        // the candidate only. Publishing live caches/save is the caller's swap.
        std::function<bool(State&)> rebuild;
        std::function<bool(State&)> recalculate_army;
    };
    static int word(int value) { return int(int16_t(uint16_t(value))); }
    // 45C3E0: WORD arithmetic precedes clamps; current/base values are independent.
    static void apply_stat_increments(Unit& u, std::array<int, 9> growth)
    {
        if (u.flags30() & 1)
        {
            for (auto& g : growth)
            {
                g = word(g + 1);
            }
        }
        auto add = [](int& v, int g, int cap)
        {
            v = std::clamp(word(v + g), 0, cap);
        };
        add(u.max_hp, growth[0], 5000);
        add(u.hp, growth[0], 5000);
        add(u.max_mp, growth[1], 999);
        add(u.mp, growth[1], 999);
        add(u.max_st, growth[2], 999);
        add(u.st, growth[2], 999);
        add(u.attack_limit, growth[3], 999);
        add(u.attack, growth[3], 999);
        add(u.defense_limit, growth[4], 999);
        add(u.defense, growth[4], 999);
        add(u.strikeback, growth[5], 999);
        add(u.base_tactics, growth[6], 999);
        add(u.tactics, growth[6], 999);
        add(u.max_sp, growth[7], 999);
        add(u.sp, growth[7], 999);
        add(u.fencing, growth[8], 999);
    }

private:
    OriginalPromotion promotion_;
    std::vector<uint8_t> kinds_, items_, bin_, pal_;
    size_t records_{ }, actions_{ }, groups_{ };
    unsigned item_count_{ }, group_count_{ };
    static unsigned w(const std::vector<uint8_t>& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dw(const std::vector<uint8_t>& b, size_t p) { return w(b, p) | (w(b, p + 2) << 16); }
    static int sb(const std::vector<uint8_t>& b, size_t p) { return int(int8_t(b.at(p))); }
    static int sw(const std::vector<uint8_t>& b, size_t p) { return int(int16_t(w(b, p))); }
    size_t kind(unsigned id) const
    {
        promotion_.candidate(id);
        return records_ + size_t(id - 1) * 37;
    }
    s2ne::Result<size_t> try_item(unsigned id) const
    {
        if (!id || id > item_count_)
        {
            return s2ne::Result<size_t>::failure(make_error("Invalid promotion equipment ID"));
        }
        return s2ne::Result<size_t>::success(2 + size_t(id - 1) * 38);
    }
    size_t item(unsigned id) const
    {
        auto result = try_item(id);
        return std::move(result).value();
    }
    bool special(const Unit& u, int value) const
    {
        for (unsigned id : u.equipment)
        {
            if (id)
            {
                size_t p = item(id);
                if (!sb(items_, p + 15) && sb(items_, p + 25) == value)
                {
                    return true;
                }
            }
        }
        return false;
    }
    bool usable(const Unit& u, unsigned id, int category) const
    {
        size_t p = item(id);
        unsigned flags = w(items_, p + 35);
        if ((flags & 0x40) && u.id != 11)
        {
            return false;
        }
        if ((flags & 0x80) && u.id != 22)
        {
            return false;
        }
        if ((flags & 0x100) && u.id != 1)
        {
            return false;
        }
        bool override = (u.abilities & 0x08000000) != 0;
        if (!(override && sb(items_, p + 14) > 6) && !(dw(items_, p + 31) & (1u << ((sb(kinds_, kind(u.appearance) + 17) - 1) & 31))))
        {
            return false;
        }
        unsigned category_mask = category == 0 ? 8 : category == 1 ? 16 :
                                                                     32;
        if (!(flags & category_mask))
        {
            return false;
        }
        unsigned attribute = u.kind_attribute + (special(u, 0x40) ? 10 : 0);
        return override || (flags & (attribute <= 30 ? 1u : attribute < 70 ? 2u :
                                                                             4u));
    }
    bool weapon(const Unit& u, unsigned id) const
    {
        size_t table = dw(kinds_, 30);
        size_t index = table + size_t(u.appearance - 1) * 4;
        int first = sw(kinds_, index), count = sw(kinds_, index + 2);
        size_t rows = table + size_t(promotion_.class_count()) * 4;
        for (int i = 0; i < count; ++i)
        {
            if (sb(kinds_, rows + size_t(first + i) * 23) == sb(items_, item(id) + 14) + 1)
            {
                return true;
            }
        }
        return false;
    }

public:
    OriginalPromotionTransaction() = default;
    static Result<OriginalPromotionTransaction> create(const std::filesystem::path& root)
    {
        auto promotion = OriginalPromotion::create(root);
        if (!promotion)
        {
            return Result<OriginalPromotionTransaction>::failure(promotion.error());
        }
        OriginalPromotionTransaction result;
        result.promotion_ = std::move(promotion).value();
        for (auto pair : { std::pair{ &result.kinds_, "KINDSP.BIN" }, std::pair{ &result.items_, "ITEMSP.BIN" }, std::pair{ &result.bin_, "SUPERSP.BIN" }, std::pair{ &result.pal_, "SUPERSP.PAL" } })
        {
            auto bytes = read_resource(root / pair.second);
            if (!bytes)
            {
                return Result<OriginalPromotionTransaction>::failure(bytes.error());
            }
            *pair.first = std::move(bytes).value();
        }
        ResourceReader kinds(result.kinds_), items(result.items_), pal(result.pal_), bin(result.bin_);
        result.records_ = kinds.number(18, 4);
        result.item_count_ = items.number(0);
        unsigned raw = pal.number(856, 4);
        if (!pal)
        {
            return Result<OriginalPromotionTransaction>::failure(pal.error());
        }
        if (raw < 12)
        {
            return Result<OriginalPromotionTransaction>::failure(make_error("Invalid promotion ability table address"));
        }
        size_t header = raw - 12;
        result.group_count_ = bin.number(header + 1, 1);
        result.groups_ = header + 9;
        result.actions_ = result.groups_ + result.group_count_ * 2;
        if (result.group_count_ > 11 || !items.extent(2, size_t(result.item_count_) * 38) || !bin.extent(result.groups_, result.group_count_ * 2))
        {
            return Result<OriginalPromotionTransaction>::failure(make_error("Invalid promotion transaction tables"));
        }
        unsigned actions = 0;
        for (unsigned group = 0; group < result.group_count_; ++group)
        {
            actions = std::max(actions, bin.number(result.groups_ + group * 2, 1) + bin.number(result.groups_ + group * 2 + 1, 1));
        }
        if (!bin.extent(result.actions_, size_t(actions) * 37))
        {
            return Result<OriginalPromotionTransaction>::failure(bin.error());
        }
        return Result<OriginalPromotionTransaction>::success(std::move(result));
    }
    explicit OriginalPromotionTransaction(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    unsigned item_count() const { return item_count_; }
    int item_defense(unsigned id) const { return sb(items_, item(id) + 21); }
    bool usable_item(const Unit& u, unsigned id, int category = 0) const { return id && id <= item_count_ && usable(u, id, category); }
    std::vector<uint8_t> item_name(unsigned id) const
    {
        size_t p = item(id);
        std::vector<uint8_t> name;
        for (size_t i = 2; i < 14 && items_.at(p + i); ++i)
        {
            name.push_back(items_[p + i]);
        }
        return name;
    }
    bool learning_item(unsigned id) const { return id && id <= item_count_ && sb(items_, item(id) + 14) == 12 && (sb(items_, item(id) + 25) == 65 || sb(items_, item(id) + 25) == 66); }
    // 451DDA: inclusive one-based ordinal range; already learned entries skip the floor.
    bool learn_range(Unit& u, unsigned group, unsigned first, unsigned last) const
    {
        if (!group || group > group_count_ || !first || first > last || last > bin_.at(groups_ + (group - 1) * 2 + 1))
        {
            return false;
        }
        --group;
        for (unsigned ordinal = first; ordinal <= last; ++ordinal)
        {
            unsigned mask = 1u << (ordinal - 1);
            if (u.learned[group] & mask)
            {
                continue;
            }
            u.learned[group] |= mask;
            size_t record = actions_ + (bin_.at(groups_ + group * 2) + ordinal - 1) * 37;
            int threshold = sw(bin_, record + 15);
            if (threshold > int(u.learned_proficiency[group]) / 10)
            {
                u.learned_proficiency[group] = uint16_t(threshold * 10);
            }
        }
        return true;
    }
    // 450FE1 effect65 teaches one ordinal; 451032 effect66 teaches 1..ordinal.
    bool use_learning_item(State& state, unsigned id, int category = 0) const
    {
        if (state.inventory.size() != item_count_ || !learning_item(id) || !state.inventory[id - 1] || !usable(state.unit, id, category))
        {
            return false;
        }
        size_t p = item(id);
        int group = sw(items_, p + 17), ordinal = sb(items_, p + 19);
        if (!learn_range(state.unit, unsigned(group), sb(items_, p + 25) == 66 ? 1u : unsigned(ordinal), unsigned(ordinal)))
        {
            return false;
        }
        --state.inventory[id - 1];
        return true;
    }
    // 441143 reads runtime WORD +8 for the ability20/40 gate. Stage behavior
    // is not an equivalent substitute; supply the original role words in order.
    Result<uint8_t> try_aggregate_army_metadata(uint8_t existing, std::span<const Unit> units,
        std::span<const int> roles8, int mode = 0, int game_mode = 0) const
    {
        (void)game_mode;
        if (mode != 0 && mode != 2)
        {
            return Result<uint8_t>::failure(make_error("Army mode1 requires stage-counter settlement, not pure metadata aggregation"));
        }
        if (roles8.size() != units.size())
        {
            return Result<uint8_t>::failure(make_error("Complete original army role8 words required"));
        }
        for (const auto& unit : units)
        {
            for (unsigned id : unit.equipment)
            {
                if (id > item_count_)
                {
                    return Result<uint8_t>::failure(make_error("Invalid army equipment ID"));
                }
            }
        }
        uint8_t result = existing & 0x81;
        for (size_t i = 0; i < units.size(); ++i)
        {
            const auto& u = units[i];
            if ((u.status_flags & 0x30) || (mode != 2 && !(u.status_flags & 0x80)))
            {
                continue;
            }
            if (u.abilities & 0x00000008)
            {
                result |= 2;
            }
            if (u.abilities & 0x00020000)
            {
                result |= 0x10;
            }
            if (u.abilities & 0x04000000)
            {
                result |= 0x0C;
            }
            if (roles8[i] != 1)
            {
                if (u.abilities & 0x00040000)
                {
                    result |= 0x20;
                }
                if (u.abilities & 0x80000000)
                {
                    result |= 0x40;
                }
            }
            for (unsigned id : u.equipment)
            {
                if (id)
                {
                    size_t p = item(id);
                    if (sb(items_, p + 15) || sb(items_, p + 25) <= 0)
                    {
                        continue;
                    }
                    if (items_.at(p + 25) == 0x1F)
                    {
                        result |= 8;
                    }
                    if (items_.at(p + 25) == 0x3A)
                    {
                        result |= 4;
                    }
                }
            }
        }
        return Result<uint8_t>::success(result);
    }
    uint8_t aggregate_army_metadata(uint8_t existing, std::span<const Unit> units, std::span<const int> roles, int mode = 0, int game_mode = 0) const
    {
        auto result = try_aggregate_army_metadata(existing, units, roles, mode, game_mode);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    std::optional<size_t> first_consumable_special(const OriginalEvents::Unit& unit, unsigned special) const
    {
        // 451D28: first matching special, accepting item+15 types0/1.
        for (size_t slot = 0; slot < unit.equipment.size(); ++slot)
        {
            if (auto id = unit.equipment[slot])
            {
                auto p = item(id);
                if (sb(items_, p + 25) == int(special) && (sb(items_, p + 15) == 0 || sb(items_, p + 15) == 1))
                {
                    return slot;
                }
            }
        }
        return std::nullopt;
    }
    bool remove_equipment(State& state, size_t slot, bool consume) const
    {
        if (slot >= state.unit.equipment.size() || state.inventory.size() != item_count_)
        {
            return false;
        }
        auto& u = state.unit;
        unsigned id = u.equipment[slot];
        if (id > item_count_)
        {
            return false;
        }
        if (!id)
        {
            return true;
        }
        size_t p = item(id);
        u.equipment[slot] = 0;
        if (!consume && state.inventory[id - 1] < 99)
        {
            ++state.inventory[id - 1];
        }
        auto subtract = [](int& v, int d)
        {
            v = std::max(word(v - d), 0);
        };
        int d = sw(items_, p + 17);
        u.equipment_stat_deltas[0] = word(u.equipment_stat_deltas[0] - d);
        subtract(u.attack, d);
        subtract(u.attack_limit, d);
        d = sb(items_, p + 21);
        u.equipment_stat_deltas[1] = word(u.equipment_stat_deltas[1] - d);
        subtract(u.defense, d);
        subtract(u.defense_limit, d);
        d = sb(items_, p + 22);
        u.equipment_stat_deltas[2] = word(u.equipment_stat_deltas[2] - d);
        // 44D31F/44D33F really clear defense rather than SP after underflow.
        u.sp = word(u.sp - d);
        if (u.sp < 0)
        {
            u.defense = 0;
        }
        u.max_sp = word(u.max_sp - d);
        if (u.max_sp < 0)
        {
            u.defense_limit = 0;
        }
        d = sb(items_, p + 24);
        u.equipment_stat_deltas[3] = word(u.equipment_stat_deltas[3] - d);
        subtract(u.max_action_points, d);
        subtract(u.action_points, d);
        subtract(u.base_action_points, d);
        return true;
    }
    bool equip(State& state, unsigned id) const
    {
        if (!id || id > item_count_)
        {
            return false;
        }
        auto& u = state.unit;
        size_t p = item(id);
        int type = sb(items_, p + 14);
        if (state.inventory.size() != item_count_)
        {
            return false;
        }
        if (type < 1 || type > 11 || !state.inventory[id - 1])
        {
            return false;
        }
        size_t slot = type <= 6 ? 0 : size_t(type - 6);
        if (u.equipment[slot])
        {
            return false;
        }
        --state.inventory[id - 1];
        u.equipment[slot] = id;
        int d = sw(items_, p + 17);
        if (u.attack + d > 999)
        {
            d = 999 - u.attack;
        }
        u.attack = word(u.attack + d);
        u.attack_limit = word(u.attack_limit + d);
        u.equipment_stat_deltas[0] = word(u.equipment_stat_deltas[0] + d);
        d = sb(items_, p + 21);
        if (u.defense + d > 999)
        {
            d = 999 - u.defense;
        }
        u.defense = word(u.defense + d);
        u.defense_limit = word(u.defense_limit + d);
        u.equipment_stat_deltas[1] = word(u.equipment_stat_deltas[1] + d);
        d = sb(items_, p + 22);
        if (u.sp + d > 999)
        {
            d = 999 - u.sp;
        }
        u.sp = word(u.sp + d);
        u.max_sp = word(u.max_sp + d);
        u.equipment_stat_deltas[2] = word(u.equipment_stat_deltas[2] + d);
        d = sb(items_, p + 24);
        if (u.base_action_points + d > 110)
        {
            d = 999 - u.base_action_points;
        }
        u.action_points = std::max(word(u.action_points + d), 0);
        u.max_action_points = std::max(word(u.max_action_points + d), 0);
        u.base_action_points = word(u.base_action_points + d);
        u.equipment_stat_deltas[3] = word(u.equipment_stat_deltas[3] + d);
        return true;
    }
    bool auto_equip(State& state, int category) const
    {
        if (state.inventory.size() != item_count_)
        {
            return false;
        }
        for (unsigned id : state.unit.equipment)
        {
            if (id > item_count_)
            {
                return false;
            }
        }
        for (size_t slot = 0; slot < 6; ++slot)
        {
            unsigned best = 0, score = 0;
            for (unsigned id = 1; id <= item_count_; ++id)
            {
                size_t p = item(id);
                int type = sb(items_, p + 14);
                if (!state.inventory[id - 1] || !(slot ? type - 6 == int(slot) : type >= 1 && type <= 6 && weapon(state.unit, id)) || !usable(state.unit, id, category))
                {
                    continue;
                }
                if (items_[p + 37] > score)
                {
                    best = id;
                    score = items_[p + 37];
                }
            }
            unsigned current = state.unit.equipment[slot];
            if (current && (!usable(state.unit, current, category) || (slot == 0 && !weapon(state.unit, current))))
            {
                remove_equipment(state, slot, false);
            }
            current = state.unit.equipment[slot];
            if (current && items_.at(item(current) + 37) > score)
            {
                continue;
            }
            if (current)
            {
                remove_equipment(state, slot, false);
            }
            if (best)
            {
                equip(state, best);
            }
        }
        if (state.unit.equipment[4] && state.unit.equipment[0] && sb(items_, item(state.unit.equipment[0]) + 14) == 4)
        {
            remove_equipment(state, 4, false);
        }
        return true;
    }
    // 45C9B1 uses one-based group and action ordinal, unlike the table APIs.
    static bool conditional_learning_allowed(const Unit& u, unsigned group, unsigned ordinal)
    {
        return group == 7 && ordinal >= 5 && ordinal <= 10 && u.learned[ordinal - 5] != 0;
    }
    bool learn_group(Unit& u, unsigned group, bool conditional = false) const
    {
        if (group >= group_count_)
        {
            return false;
        }
        unsigned first = bin_.at(groups_ + group * 2), count = bin_.at(groups_ + group * 2 + 1);
        for (unsigned bit = 0; bit < count; ++bit)
        {
            size_t p = actions_ + size_t(first + bit) * 37;
            if (sw(bin_, p + 15) > int(u.learned_proficiency[group] / 10))
            {
                continue;
            }
            if ((bin_.at(p + 32) & 0x80) && (!conditional || !conditional_learning_allowed(u, group + 1, bit + 1)))
            {
                continue;
            }
            u.learned[group] |= uint16_t(1u << (bit & 31));
        }
        return true;
    }
    void learn_relations(Unit& u) const
    {
        if (u.learned[6] && group_count_ > 6)
        {
            unsigned first = bin_.at(groups_ + 12) + 4;
            for (unsigned i = 0; i < 6; ++i)
            {
                size_t p = actions_ + size_t(first + 2 * i) * 37;
                if (sw(bin_, p + 15) <= int(u.learned_proficiency[6] / 10) && !(u.learned[6] & (1u << i)) && u.learned[i])
                {
                    u.learned[6] |= 1u << (i + 4);
                }
            }
        }
    }
    // 4659BD: index is the original WORD +D7 array index, not group ID.
    // 0..5 weapon proficiency, 6..16 learned-group proficiency +E3.
    bool award_proficiency(Unit& u, unsigned index, unsigned increment) const
    {
        if (index >= 6 + group_count_ || increment > 65535)
        {
            return false;
        }
        auto next = u;
        unsigned& value = index < 6 ? next.weapon_proficiency[index] : next.learned_proficiency[index - 6];
        if (value >= 15000)
        {
            return false;
        }
        value = std::min(unsigned(uint16_t(value + increment)), 15000u);
        if (index >= 6)
        {
            learn_group(next, index - 6, true);
        }
        u = std::move(next);
        return true;
    }
    void train(Unit& u, int group) const
    {
        if (group >= 0 && group < 11)
        {
            u.learned_proficiency[size_t(group)] = std::min(unsigned(uint16_t(u.learned_proficiency[size_t(group)] + 300)), 15000u);
            if (unsigned(group) < group_count_)
            {
                learn_group(u, unsigned(group));
            }
        }
        learn_relations(u);
    }
    std::optional<State> prepare(const State& before, const OriginalPromotion::Request& request, const Context& context,
        const OriginalRandom::TimeSource& time = { }) const
    {
        if (!context.rebuild || !context.recalculate_army || before.inventory.size() != item_count_)
        {
            return std::nullopt;
        }
        for (unsigned id : before.unit.equipment)
        {
            if (id > item_count_)
            {
                return std::nullopt;
            }
        }
        State next = before;
        auto& u = next.unit;
        if (u.id != request.unit_id || u.appearance != request.source_kind || !promotion_.eligible(u, u.experience_threshold, context.game_mode))
        {
            return std::nullopt;
        }
        auto choices = promotion_.candidates(u);
        auto chosen = std::find_if(choices.begin(), choices.end(), [&](const auto& c)
            {
                return c.kind == request.target_kind;
            });
        if (chosen == choices.end() || request.training_group < -1 || request.training_group >= 11 || (chosen->training_mask ? request.training_group < 0 || !(chosen->training_mask & (1u << request.training_group)) : request.training_group != -1))
        {
            return std::nullopt;
        }
        for (size_t i = 0; i < 6; ++i)
        {
            if (chosen->required_item && u.equipment[i] == chosen->required_item)
            {
                remove_equipment(next, i, true);
            }
        }
        for (size_t i = 0; i < 6; ++i)
        {
            remove_equipment(next, i, false);
        }
        size_t old_kind = kind(u.appearance), new_kind = kind(request.target_kind);
        u.kind_attribute = std::min(110u, (u.kind_attribute > kinds_.at(old_kind + 26) ? u.kind_attribute - kinds_[old_kind + 26] : 0) + kinds_.at(new_kind + 26));
        auto growthResult = promotion_.try_growth(request.target_kind, 1, next.random, time);
        if (!growthResult)
        {
            report_error(growthResult.error());
            return std::nullopt;
        }
        auto growth = growthResult.value(), baseline = promotion_.baseline(request.target_kind);
        std::array<int, 9> values{ u.max_hp, u.max_mp, u.max_st, u.attack_limit, u.defense_limit, u.strikeback, u.base_tactics, u.max_sp, u.fencing };
        for (size_t i = 0; i < 9; ++i)
        {
            if (values[i] < baseline[i])
            {
                growth[i] = word(growth[i] + (baseline[i] - values[i]) / 2);
            }
        }
        apply_stat_increments(u, growth);
        int ap = std::max(sb(kinds_, new_kind + 21) - u.base_action_points, 0);
        if (sb(kinds_, old_kind + 17) == 5 && sb(kinds_, new_kind + 17) != 5)
        {
            ap = -4;
        }
        if (u.base_action_points + ap < 110)
        {
            u.base_action_points = word(u.base_action_points + ap);
            u.max_action_points = word(u.max_action_points + ap);
        }
        if (request.battlefield)
        {
            u.action_points = word(u.action_points + ap);
        }
        if (u.action_points <= 0)
        {
            u.action_points = u.carry_points = 0;
        }
        if (kinds_.at(new_kind + 19))
        {
            u.abilities |= 1u << ((sb(kinds_, new_kind + 19) - 1) & 31);
        }
        u.level = 1;
        u.experience = 0;
        u.experience_threshold = promotion_.experience_threshold(request.target_kind);
        train(u, request.training_group);
        u.appearance = request.target_kind;
        u.movement_type = sb(kinds_, new_kind + 18);
        u.profession = chosen->name;
        if (!u.id || u.id >= 200)
        {
            u.portrait = kinds_.at(new_kind + 13);
        }
        if (!context.rebuild(next))
        {
            return std::nullopt;
        }
        if (!auto_equip(next, int(int8_t(u.allegiance_tag))))
        {
            return std::nullopt;
        }
        if (!context.recalculate_army(next))
        {
            return std::nullopt;
        }
        return next;
    }
};
}    //namespace s2ne
