#include "combat.h"

namespace s2ne
{
Random::Random(uint32_t seed) : state_(seed) {}

auto Random::seed(uint32_t value) -> void { state_ = value; }

auto Random::raw() -> unsigned
{
    state_ = state_ * 0x41C64E6Du + 0x3039u;
    return (state_ >> 16) & 0x7FFFu;
}

auto Random::bounded(int bound, uint32_t seconds) -> int
{
    if (!bound)
    {
        return 0;
    }
    before_draw(seconds);
    return int(raw()) % bound;
}

auto Random::initialize(uint32_t seconds) -> void
{
    initialize_with_clock([seconds]
        {
            return seconds;
        });
}

auto Random::percent(uint32_t seconds) -> int
{
    before_draw(seconds);
    return table_[raw() % 100];
}

auto Random::draw_count() const -> unsigned { return draws_; }

auto Random::seed_state() const -> uint32_t { return state_; }

auto Random::restore_stream(uint32_t seed, unsigned calls) -> void
{
    state_ = seed;
    draws_ = calls;
}

auto Random::table() const -> const std::array<uint8_t, 100>& { return table_; }

auto Random::before_draw(uint32_t seconds) -> void
{
    if (++draws_ >= 200)
    {
        seed(seconds);
        draws_ = 0;
    }
}

auto try_decode_combat_value(std::span<const uint8_t> unit, std::span<const uint8_t> config,
    std::span<const uint8_t> kind, unsigned first_group_end, int mode, bool equipment18) -> Result<CombatValueInput>
{
    if (unit.size() < 0x13A || config.size() <= 0xDF || kind.size() <= 0x21 || first_group_end > 11)
    {
        return Result<CombatValueInput>::failure(make_error("original combat value record"));
    }
    auto word = [&](size_t p)
    {
        return int(int16_t(unsigned(unit[p]) | (unsigned(unit[p + 1]) << 8)));
    };
    CombatValueInput in{};
    int valueOffset;
    if (mode == 2) { valueOffset = 0xB7; }
    else if (mode == 1) { valueOffset = 0x132; }
    else
    {
        valueOffset = 0x124;
    }
    in.value18 = word(valueOffset);
    in.value14 = word(mode == 2 ? 0xBD : 0x12A);
    in.value10 = word(mode == 2 ? 0xB9 : 0x126);
    in.valueC = word(mode == 2 ? 0xBB : 0x128);
    in.runtime134 = word(0x134);
    in.runtime136 = word(0x136);
    in.runtime138 = word(0x138);
    in.runtimeC1 = word(0xC1);
    in.runtimeBF = word(0xBF);
    in.runtimeC7 = word(0xC7);
    for (unsigned i = 0; i < 11; ++i)
    {
        if (word(0xFB + i * 2) != 0)
        {
            if (i < first_group_end)
            {
                ++in.learned_first;
            }
            else
            {
                ++in.learned_second;
            }
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        in.abilities |= uint32_t(unit[0x115 + i]) << (i * 8);
    }
    in.kind_multiplier = kind[0x21];
    in.equipment_18 = equipment18;
    in.configD6 = int8_t(config[0xD6]);
    in.configD7 = int8_t(config[0xD7]);
    in.configD8 = int8_t(config[0xD8]);
    in.configD9 = int8_t(config[0xD9]);
    in.configDA = int8_t(config[0xDA]);
    in.configDB = int8_t(config[0xDB]);
    in.configDC = int8_t(config[0xDC]);
    in.configDF = int8_t(config[0xDF]);
    return Result<CombatValueInput>::success(in);
}

auto try_combat_value_parts(const CombatValueInput& in) -> Result<std::array<int, 4>>
{
    if (!in.configD6 || !in.configD7 || !in.configD8 || !in.configD9 || !in.configDA || !in.configDB || !in.configDC)
    {
        return Result<std::array<int, 4>>::failure(make_error("original combat value divisor"));
    }
    int value14 = in.value14 + (in.equipment_18 ? in.value14 / 2 : 0);
    unsigned count = 0;
    for (uint32_t mask = 1; mask; mask <<= 1)
    {
        if ((in.abilities & mask) && !(mask & (0x40u | 0x20000u | 0x40000u | 0x200000u | 0x4000000u | 0x8000000u | 0x80000000u)))
        {
            ++count;
        }
    }
    return Result<std::array<int, 4>>::success({ value14 * (in.learned_first + 10) * (in.value10 + 1) / (in.configD6 * 10 * in.configD7),
        in.runtimeC7 * (in.learned_second + 10) * (in.valueC + 1) / (in.configD8 * 10 * in.configD9),
        (in.runtime134 + in.runtime136) * (in.runtime138 + in.runtimeC1) / in.configDB / in.configDC + in.value18 / in.configDA,
        int(std::sqrt(double(count)) * 400 * count) });
}

auto try_combat_value(const CombatValueInput& in) -> Result<int>
{
    if (!in.configDF)
    {
        return Result<int>::failure(make_error("original combat value scale"));
    }
    auto result = try_combat_value_parts(in);
    if (!result)
    {
        return Result<int>::failure(result.error());
    }
    const auto& parts = result.value();
    return Result<int>::success(std::max(1, (parts[0] + parts[1] + parts[2] + parts[3]) * in.runtimeBF / in.configDF * in.kind_multiplier / 10));
}

auto decode_combat_value(std::span<const uint8_t> unit, std::span<const uint8_t> config, std::span<const uint8_t> kind, unsigned first_group_end, int mode, bool equipment18) -> CombatValueInput
{
    auto result = try_decode_combat_value(unit, config, kind, first_group_end, mode, equipment18);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto combat_value_parts(const CombatValueInput& in) -> std::array<int, 4>
{
    auto result = try_combat_value_parts(in);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto combat_value(const CombatValueInput& in) -> int
{
    auto result = try_combat_value(in);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto action_status(const ActionStatusInput& in) -> ActionStatusResult
{
    ActionStatusResult out{ 0, 0, 0, 0, in.result };
    if (in.result <= 0 || in.target_status33_20 || in.target_status3e_10 || in.target_116_40)
    {
        return out;
    }
    int chance = in.target_equipment14 ? in.chance / 2 : in.chance;
    if ((in.target_kind & 3) && in.effect == 8)
    {
        chance = chance * 7 / 10;
    }
    if (in.roll > chance)
    {
        return out;
    }
    out.duration = in.duration;
    switch (in.effect)
    {
    case 1:
        if (!in.target_116_10 && in.target_kind != 1)
        {
            out.function = 0x442F74;
        }
        break;
    case 2:
        if (in.target_kind != 1)
        {
            out.function = 0x442CD5;
            out.mask = 0x8000;
        }
        break;
    case 3:
        if (!(in.target_kind & 3))
        {
            out.function = 0x442FE2;
        }
        break;
    case 4:
        if (!(in.target_kind & 3))
        {
            out.function = 0x442CD5;
            out.mask = 0x1000;
        }
        break;
    case 5:
        out.function = 0x443093;
        out.amount = in.target_runtime134 / 2;
        break;
    case 6:
        if (in.target_kind != 1)
        {
            out.function = 0x442CD5;
            out.mask = 0x10000;
        }
        break;
    case 7:
        if (out.result > 0)
        {
            out.result = -out.result;
        }
        break;
    case 8:
        out.function = 0x442CD5;
        out.mask = 0x2000;
        break;
    case 9:
        if (in.target_kind17 == 14 && in.target_kind != 1)
        {
            out.result = in.target_runtimeB7;
        }
        break;
    case 10:
        if (!(in.target_kind & 3))
        {
            out.result = in.target_runtime124;
        }
        break;
    case 11:
        out.function = 0x442CD5;
        out.mask = 0x40000;
        break;
    }
    return out;
}

auto try_apply_timed_status(std::span<uint8_t> unit, uint32_t mask, int duration) -> Result<bool>
{
    if (unit.size() < 0xA0)
    {
        return Result<bool>::failure(make_error("original timed status record"));
    }
    unsigned slot = 0;
    uint32_t shifted = mask;
    for (unsigned bit = 0; bit < 27; ++bit)
    {
        if (shifted & 1)
        {
            slot = bit;
            break;
        }
        shifted >>= 1;
    }
    uint32_t flags = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        flags |= uint32_t(unit[0x3C + i]) << (i * 8);
    }
    bool inserted = (flags & mask) == 0;
    auto& stored = unit[0x85 + slot];
    if (inserted)
    {
        flags |= mask;
        stored = uint8_t(duration);
        for (unsigned i = 0; i < 4; ++i)
        {
            unit[0x3C + i] = uint8_t(flags >> (i * 8));
        }
    }
    else if (int(int8_t(stored)) < duration && int8_t(stored) != -1)
    {
        stored = uint8_t(duration);
    }
    return Result<bool>::success(inserted);
}

auto apply_timed_status(std::span<uint8_t> unit, uint32_t mask, int duration) -> bool
{
    auto result = try_apply_timed_status(unit, mask, duration);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto try_apply_stat_status(std::span<uint8_t> unit, uint32_t mask, int duration, int delta134, int delta136) -> Result<bool>
{
    if (unit.size() < 0x138)
    {
        return Result<bool>::failure(make_error("original stat status record"));
    }
    unsigned slot = 0;
    uint32_t shifted = mask >> 9;
    for (unsigned bit = 0; bit < 5; ++bit)
    {
        if (shifted & 1)
        {
            slot = bit;
            break;
        }
        shifted >>= 1;
    }
    uint32_t flags = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        flags |= uint32_t(unit[0x38 + i]) << (i * 8);
    }
    auto& stored = unit[0x63 + slot * 5];
    if (flags & mask)
    {
        if (int(int8_t(stored)) < duration && int8_t(stored) != -1)
        {
            stored = uint8_t(duration);
        }
        return Result<bool>::success(false);
    }
    flags |= mask;
    for (unsigned i = 0; i < 4; ++i)
    {
        unit[0x38 + i] = uint8_t(flags >> (i * 8));
    }
    auto word = [&](size_t p)
    {
        return unsigned(unit[p]) | (unsigned(unit[p + 1]) << 8);
    };
    auto put = [&](size_t p, unsigned value)
    {
        unit[p] = uint8_t(value);
        unit[p + 1] = uint8_t(value >> 8);
    };
    put(0x64 + slot * 5, unsigned(delta134));
    put(0x66 + slot * 5, unsigned(delta136));
    put(0x134, word(0x134) + unsigned(delta134));
    put(0x136, word(0x136) + unsigned(delta136));
    stored = uint8_t(duration);
    return Result<bool>::success(true);
}

auto apply_stat_status(std::span<uint8_t> unit, uint32_t mask, int duration, int delta134, int delta136) -> bool
{
    auto result = try_apply_stat_status(unit, mask, duration, delta134, delta136);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto try_apply_stat134_status(std::span<uint8_t> unit, int amount, int duration) -> Result<bool>
{
    if (unit.size() < 0x136)
    {
        return Result<bool>::failure(make_error("original single stat status record"));
    }
    auto& stored = unit[0x4B];
    if (unit[0x38] & 2)
    {
        if (int(int8_t(stored)) < duration && int8_t(stored) != -1)
        {
            stored = uint8_t(duration);
        }
        return Result<bool>::success(false);
    }
    unit[0x38] |= 2;
    unsigned value = unsigned(unit[0x134]) | (unsigned(unit[0x135]) << 8);
    value += unsigned(amount);
    unit[0x134] = uint8_t(value);
    unit[0x135] = uint8_t(value >> 8);
    unit[0x4C] = uint8_t(amount);
    unit[0x4D] = uint8_t(unsigned(amount) >> 8);
    stored = uint8_t(duration);
    return Result<bool>::success(true);
}

auto apply_stat134_status(std::span<uint8_t> unit, int amount, int duration) -> bool
{
    auto result = try_apply_stat134_status(unit, amount, duration);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto try_recalculate_status_stats(std::span<uint8_t> unit) -> Result<bool>
{
    if (unit.size() < 0x13A)
    {
        return Result<bool>::failure(make_error("original status stats record"));
    }
    auto word = [&](size_t p)
    {
        return unsigned(unit[p]) | (unsigned(unit[p + 1]) << 8);
    };
    auto put = [&](size_t p, unsigned value)
    {
        unit[p] = uint8_t(value);
        unit[p + 1] = uint8_t(value >> 8);
    };
    for (auto pair : std::array<std::array<size_t, 2>, 5>{ { { 0x12A, 0xBD }, { 0x12C, 0xBF }, { 0x134, 0xC3 }, { 0x136, 0xC5 }, { 0x138, 0xC9 } } })
    {
        put(pair[0], word(pair[1]));
    }
    uint32_t flags = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        flags |= uint32_t(unit[0x38 + i]) << (i * 8);
    }
    if (!flags)
    {
        return Result<bool>::success(false);
    }
    constexpr std::array<size_t, 9> destinations{ 0x136, 0x134, 0x134, 0x134, 0x136, 0x12A, 0x12C, 0x12C, 0x138 };
    for (unsigned i = 0; i < 9; ++i)
    {
        if (flags & (1u << i))
        {
            put(destinations[i], word(destinations[i]) + word(0x49 + i * 3));
        }
    }
    for (unsigned i = 0; i < 5; ++i)
    {
        if (flags & (0x200u << i))
        {
            put(0x134, word(0x134) + word(0x64 + i * 5));
            put(0x136, word(0x136) + word(0x66 + i * 5));
        }
    }
    if (flags & 0x4000)
    {
        put(0x12C, word(0x12C) + word(0x7D));
        put(0x134, word(0x134) + word(0x7F));
        put(0x136, word(0x136) + word(0x81));
    }
    if (int16_t(word(0x12C)) < 0)
    {
        put(0x12C, 0);
    }
    if (int16_t(word(0x134)) < 0)
    {
        put(0x134, 0);
    }
    if (int16_t(word(0x136)) < 0)
    {
        put(0x136, 0);
    }

    return Result<bool>::success(true);
}

auto recalculate_status_stats(std::span<uint8_t> unit) -> void
{
    auto result = try_recalculate_status_stats(unit);
    if (!result)
    {
        fatal(result.error());
    }
}

auto try_apply_action_status(std::span<uint8_t> unit, ActionStatusInput input) -> Result<ActionStatusResult>
{
    if (unit.size() < 0x13A)
    {
        return Result<ActionStatusResult>::failure(make_error("original action status record"));
    }
    auto word = [&](size_t p)
    {
        return int(int16_t(unsigned(unit[p]) | (unsigned(unit[p + 1]) << 8)));
    };
    input.target_kind = unit[0x2F];
    input.target_status33_20 = (unit[0x33] & 0x20) != 0;
    input.target_status3e_10 = (unit[0x3E] & 0x10) != 0;
    input.target_116_40 = (unit[0x116] & 0x40) != 0;
    input.target_116_10 = (unit[0x116] & 0x10) != 0;
    input.target_runtime134 = word(0x134);
    input.target_runtimeB7 = word(0xB7);
    input.target_runtime124 = word(0x124);
    auto result = action_status(input);
    switch (result.function)
    {
    case 0x442CD5: apply_timed_status(unit, result.mask, result.duration); break;
    case 0x442F74: apply_stat_status(unit, 0x400, result.duration, -word(0x134) / 5, -word(0x136) / 5); break;
    case 0x442FE2: result.redraw = apply_stat_status(unit, 0x200, result.duration, -word(0x134) / 2, -word(0x136) / 2); break;
    case 0x443093: apply_stat134_status(unit, result.amount, result.duration); break;
    }
    return Result<ActionStatusResult>::success(result);
}

auto apply_action_status(std::span<uint8_t> unit, ActionStatusInput input) -> ActionStatusResult
{
    auto result = try_apply_action_status(unit, input);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto try_remove_one_status(std::span<uint8_t> unit) -> Result<uint32_t>
{
    if (unit.size() < 0x13A)
    {
        return Result<uint32_t>::failure(make_error("original status removal record"));
    }
    auto read = [&](size_t p)
    {
        uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            v |= uint32_t(unit[p + i]) << (i * 8);
        }
        return v;
    };
    auto write = [&](size_t p, uint32_t v)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            unit[p + i] = uint8_t(v >> (i * 8));
        }
    };
    uint32_t stats = read(0x38), timed = read(0x3C), removed = 0;
    if (stats & 0x3974)
    {
        for (uint32_t mask : std::array<uint32_t, 8>{ 4, 0x10, 0x20, 0x40, 0x100, 0x800, 0x1000, 0x2000 })
        {
            if (stats & mask)
            {
                write(0x38, stats ^ mask);
                removed = mask;
                break;
            }
        }
    }
    else if (timed & 0x6F00FFF)
    {
        for (uint32_t mask : std::array<uint32_t, 18>{ 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x100000, 0x200000, 0x400000, 0x800000, 0x2000000, 0x4000000 })
        {
            if (timed & mask)
            {
                write(0x3C, timed ^ mask);
                removed = mask;
                break;
            }
        }
    }
    else
    {
        return Result<uint32_t>::success(0);
    }
    recalculate_status_stats(unit);
    return Result<uint32_t>::success(removed);
}

auto remove_one_status(std::span<uint8_t> unit) -> uint32_t
{
    auto result = try_remove_one_status(unit);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto try_tick_status_durations(std::span<uint8_t> unit) -> Result<bool>
{
    if (unit.size() < 0x13A)
    {
        return Result<bool>::failure(make_error("original status countdown record"));
    }
    if (!(unit[0x33] & 0x80) || (unit[0x33] & 0x20))
    {
        return Result<bool>::success(false);
    }
    auto read = [&](size_t p)
    {
        uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            v |= uint32_t(unit[p + i]) << (i * 8);
        }
        return v;
    };
    auto write = [&](size_t p, uint32_t v)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            unit[p + i] = uint8_t(v >> (i * 8));
        }
    };
    uint32_t stats = read(0x38);
    if (stats)
    {
        for (unsigned bit = 0; bit < 15; ++bit)
        {
            unsigned int p;
            if (bit < 9) { p = 0x48 + bit * 3; }
            else if (bit < 14) { p = 0x63 + (bit - 9) * 5; }
            else
            {
                p = 0x7C;
            }

            if ((stats & (1u << bit)) && int8_t(unit[p]) > 0)
            {
                --unit[p];
                if (int8_t(unit[p]) <= 0)
                {
                    stats ^= 1u << bit;
                }
            }
        }
        write(0x38, stats);
        recalculate_status_stats(unit);
    }
    uint32_t timed = read(0x3C);
    for (unsigned bit = 0; bit < 27; ++bit)
    {
        auto& duration = unit[0x85 + bit];
        if ((timed & (1u << bit)) && int8_t(duration) > 0)
        {
            --duration;
            if (int8_t(duration) <= 0)
            {
                timed ^= 1u << bit;
            }
        }
    }
    write(0x3C, timed);

    return Result<bool>::success(true);
}

auto tick_status_durations(std::span<uint8_t> unit) -> void
{
    auto result = try_tick_status_durations(unit);
    if (!result)
    {
        fatal(result.error());
    }
}

auto try_adjust_runtime_stats(std::span<uint8_t> unit, int percentage) -> Result<bool>
{
    if (!percentage)
    {
        return Result<bool>::success(false);
    }
    if (unit.size() < 0x13A)
    {
        return Result<bool>::failure(make_error("original adjusted stats record"));
    }
    auto word = [&](size_t p)
    {
        return int(int16_t(unsigned(unit[p]) | (unsigned(unit[p + 1]) << 8)));
    };
    auto put = [&](size_t p, int value)
    {
        unit[p] = uint8_t(value);
        unit[p + 1] = uint8_t(unsigned(value) >> 8);
    };
    auto add = [&](size_t p, int delta)
    {
        put(p, word(p) + delta);
    };
    auto limit = [&](size_t preceding, size_t destination, int upper, bool minimum)
    {
        int value = word(preceding);
        if (value > upper)
        {
            put(destination, upper);
        }
        else if (minimum && value < 1)
        {
            put(destination, 1);
        }
    };
    int delta = word(0xB7) * percentage / 100;
    add(0xB7, delta);
    add(0x124, delta);
    limit(0xB7, 0xB7, 5000, true);
    limit(0x124, 0x124, 5000, true);
    delta = word(0xB9) * percentage / 100;
    add(0xB9, delta);
    add(0x126, delta);
    limit(0xB9, 0xB9, 999, false);
    limit(0x126, 0x126, 999, false);
    delta = word(0xBB) * percentage / 100;
    add(0xBB, delta);
    add(0x128, delta);
    limit(0xBB, 0xBB, 999, false);
    limit(0x128, 0x128, 999, false);
    delta = word(0xC3) * percentage / 100;
    add(0xC3, delta);
    add(0x134, delta);
    limit(0xC3, 0xC3, 999, true);
    limit(0x134, 0x134, 999, true);
    delta = word(0xC5) * percentage / 100;
    add(0xC5, delta);
    add(0x136, delta);
    limit(0xC5, 0xC5, 999, true);
    limit(0x136, 0x136, 999, true);
    delta = word(0xC7) * percentage / 100;
    add(0xC7, delta);
    limit(0xC7, 0xC7, 999, true);
    delta = word(0xC9) * percentage / 100;
    add(0xC9, delta);
    add(0x138, delta);
    limit(0xC9, 0xC9, 999, true);
    limit(0x138, 0x138, 999, true);

    return Result<bool>::success(true);
}

auto adjust_runtime_stats(std::span<uint8_t> unit, int percentage) -> void
{
    auto result = try_adjust_runtime_stats(unit, percentage);
    if (!result)
    {
        fatal(result.error());
    }
}

auto stage_stat_adjustment(int average_value, unsigned baseline, unsigned divisor, int difficulty) -> int
{
    if (!baseline || !divisor)
    {
        return 0;
    }
    int step = (average_value - int(baseline)) / int(divisor);
    if (!step)
    {
        return 0;
    }
    step = std::clamp(step, -4, 4);
    return (step > 0 ? step * 25 : step * -10) + (difficulty - 3) * 5;
}

auto try_action_experience(const ExperienceInput& in) -> Result<int>
{
    if (in.result == 0 && !in.target_status33_20)
    {
        return Result<int>::success(1);
    }
    if (!in.configE1 || (!in.target_alive && !in.source_value) || in.source_value + in.target_value == 0)
    {
        return Result<int>::failure(make_error("original experience divisor"));
    }
    int value = std::max(1, in.result * in.target_value * in.configE5 / (in.target_value + in.source_value) / in.configE1);
    if (!in.target_alive)
    {
        value += in.configE2 * in.target_value / in.source_value + 3;
    }
    return Result<int>::success(value);
}

auto action_experience(const ExperienceInput& in) -> int
{
    auto result = try_action_experience(in);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto try_ordinary_damage(int attacker_attack, int defender_defense,
    int divisor, int minimum_roll) -> Result<int>
{
    if (divisor <= 0)
    {
        return Result<int>::failure(make_error("original damage divisor"));
    }
    int value = (attacker_attack - defender_defense) * 100 / divisor;
    if (value > 100)
    {
        if (value <= 200)
        {
            value = 100 + (value - 100) * 90 / 100;
        }
        else if (value <= 300)
        {
            value = 190 + (value - 200) * 80 / 100;
        }
        else if (value <= 400)
        {
            value = 270 + (value - 300) * 70 / 100;
        }
        else
        {
            value = 340 + (value - 400) * 60 / 100;
        }
    }
    if (value < 3)
    {
        int minimumDamage;
        if (minimum_roll < 1) { minimumDamage = 5; }
        else if (minimum_roll < 6) { minimumDamage = 4; }
        else if (minimum_roll < 16) { minimumDamage = 3; }
        else if (minimum_roll < 46) { minimumDamage = 2; }
        else
        {
            minimumDamage = 1;
        }

        value = minimumDamage;
    }
    return Result<int>::success(value);
}

auto ordinary_damage(int attacker_attack, int defender_defense,
    int divisor, int minimum_roll) -> int
{
    auto result = try_ordinary_damage(attacker_attack, defender_defense, divisor, minimum_roll);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto modify_damage(const DamageModifierInput& in) -> int
{
    int matchup = in.class_matchup;
    if (matchup > 0 && (in.defender_116 & 8))
    {
        matchup -= matchup / 2;
    }
    int value = in.damage * (100 + matchup + in.attack_mode_bonus * 10) / 100;
    value = value * in.action_coefficient / 10;
    if (value > 100)
    {
        if (value <= 200)
        {
            value = 100 + (value - 100) * 90 / 100;
        }
        else if (value <= 300)
        {
            value = 190 + (value - 200) * 80 / 100;
        }
        else if (value <= 400)
        {
            value = 270 + (value - 300) * 70 / 100;
        }
        else
        {
            value = 340 + (value - 400) * 60 / 100;
        }
    }
    value += std::min(15, in.combat_value / 1000);
    if ((in.defender_3f & 4) || in.defender_equipment_13)
    {
        value -= value / 3;
    }
    if (in.defender_3c & 0x20)
    {
        value = value > 150 ? value - 150 : 1;
    }
    if (in.defender_3c & 0x40)
    {
        value = value > 180 ? value - 180 : 1;
    }
    if (in.attacker_3e & 4)
    {
        value /= 2;
    }
    if (in.defender_39 & 0x10)
    {
        value = value * 80 / 100;
    }
    if (in.defender_39 & 0x20)
    {
        value = value * 70 / 100;
    }
    return value;
}

auto damage_divisor(int percent_roll, int decimal_roll) -> int
{
    int base;
    if (percent_roll < 2)
    {
        if (decimal_roll >= 5) { base = 140; }
        else
        {
            base = 100;
        }
    }
    else if (percent_roll < 10)
    {
        if (decimal_roll >= 5) { base = 135; }
        else
        {
            base = 105;
        }
    }
    else if (percent_roll < 25)
    {
        if (decimal_roll >= 5) { base = 130; }
        else
        {
            base = 110;
        }
    }
    else if (percent_roll < 50)
    {
        if (decimal_roll >= 5) { base = 125; }
        else
        {
            base = 115;
        }
    }
    else
    {
        base = 120;
    }

    return std::max(1, base + decimal_roll);
}

auto try_physical_damage(const PhysicalDamageInput& in) -> Result<int>
{
    if (in.divisor <= 0)
    {
        return Result<int>::failure(make_error("original damage divisor"));
    }
    int height = std::clamp(in.height_difference, -8, 4), scale = height >= 0 ? 250 : 125;
    int defense = (((in.attacker_3c & 0x80) || (in.attacker_118 & 0x40)) && in.defender_kind != 1) ? 0 : in.defense;
    int factor = height * scale + 1000 + (in.attacker_terrain_bonus - in.defender_terrain_bonus) * 10 + in.context_bonus;
    auto modifiers = in.modifiers;
    modifiers.damage = (in.attack - defense) * factor / 10 / in.divisor;
    int attackModeBonus;
    if (in.attack_mode == 3) { attackModeBonus = 6; }
    else if (in.attack_mode == 2) { attackModeBonus = 3; }
    else
    {
        attackModeBonus = 0;
    }

    modifiers.attack_mode_bonus = attackModeBonus;
    int value = modify_damage(modifiers);
    if (in.height_difference > 0)
    {
        value += in.height_difference;
    }
    value = value * in.random_multiplier / 10;
    if (!in.multiplayer && !in.player_side)
    {
        value = value * (100 + in.difficulty_bonus) / 100;
    }
    value = std::max(1, value);
    if (value < 3)
    {
        int minimumDamage;
        if (in.minimum_roll < 1) { minimumDamage = 5; }
        else if (in.minimum_roll < 6) { minimumDamage = 4; }
        else if (in.minimum_roll < 16) { minimumDamage = 3; }
        else if (in.minimum_roll < 46) { minimumDamage = 2; }
        else
        {
            minimumDamage = 1;
        }

        value = minimumDamage;
    }
    return Result<int>::success(value);
}

auto physical_damage(const PhysicalDamageInput& in) -> int
{
    auto result = try_physical_damage(in);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto preview_multiplier(int base, int spread) -> int { return base + (spread + 3) / 2; }

auto physical_preview(const PhysicalPreviewInput& in) -> int
{
    int height = std::clamp(in.height_difference, -8, 4);
    int factor = 1000 + height * (height >= 0 ? 250 : 125) + (in.attacker_terrain_bonus - in.defender_terrain_bonus) * 10 + in.context_bonus;
    auto modifiers = in.modifiers;
    int defense = (((in.attacker_3c & 0x80) || (in.attacker_118 & 0x40)) && in.defender_kind != 1) ? 0 : in.defense;
    modifiers.damage = (in.attack - defense) * factor / 1000 * 100 / 125;
    modifiers.attack_mode_bonus = in.mode_bonus;
    int value = modify_damage(modifiers);
    if (in.counter)
    {
        value = value * 35 / 100;
    }
    if (in.height_difference > 0)
    {
        value += in.height_difference;
    }
    if (in.counter_fixed_damage)
    {
        value = in.counter_fixed_value;
    }
    if (in.counter_equipment_30)
    {
        value *= 3;
    }
    value = value * in.multiplier / 10;
    if (!in.multiplayer && !in.player_side)
    {
        value = value * (100 + in.difficulty_bonus) / 100;
    }
    value = std::max(1, value);
    return in.immune ? 0 : value;
}

auto try_lost_health_percent(int maximum_hp, int hp) -> Result<int>
{
    if (!maximum_hp)
    {
        return Result<int>::failure(make_error("original #E048 zero maximum HP"));
    }
    return Result<int>::success((maximum_hp - hp) * 100 / maximum_hp);
}

auto lost_health_percent(int maximum_hp, int hp) -> int
{
    auto result = try_lost_health_percent(maximum_hp, hp);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto adjusted_tactics(int value, int condition, int percent_roll) -> int
{
    if (condition > 70 && condition <= 90)
    {
        return value * (percent_roll >= 90 ? 3 : 13) / 10;
    }
    if (condition > 90)
    {
        int half = value / 2 - (value < 0 && value % 2 != 0 ? 1 : 0);
        return percent_roll >= 95 ? half : value + half;
    }
    return value;
}

auto modify_hit_chance(const HitModifierInput& in) -> int
{
    int value = in.chance + (in.action_type >= 21 ? in.action_bonus : in.action_bonus * in.weapon_accuracy / 100);
    value -= in.defender_equipment_penalty;
    if (in.defender_115 & 2)
    {
        value -= 30;
    }
    if ((in.defender_3c & 4) || in.defender_equipment_16)
    {
        value -= value / 10;
    }
    return std::clamp(value, 1, 100);
}

auto try_counter_damage(const CounterDamageInput& in) -> Result<int>
{
    const auto& p = in.physical;
    if (p.divisor <= 0)
    {
        return Result<int>::failure(make_error("original counter damage divisor"));
    }
    int height = std::clamp(p.height_difference, -8, 4);
    int defense = (((p.attacker_3c & 0x80) || (p.attacker_118 & 0x40)) && p.defender_kind != 1) ? 0 : p.defense;
    int factor = 1000 + height * (height >= 0 ? 250 : 125) + (p.attacker_terrain_bonus - p.defender_terrain_bonus) * 10 + p.context_bonus;
    auto modifiers = p.modifiers;
    modifiers.damage = (p.attack - defense) * factor / 10 / p.divisor;
    int attackModeBonus;
    if (p.attack_mode == 3) { attackModeBonus = -6; }
    else if (p.attack_mode == 2) { attackModeBonus = -3; }
    else
    {
        attackModeBonus = 0;
    }

    modifiers.attack_mode_bonus = attackModeBonus;
    int value = modify_damage(modifiers) * (in.reduction_roll + 2) / 10;
    if (p.height_difference > 0)
    {
        value += p.height_difference;
    }
    if (in.fixed_damage)
    {
        value = in.fixed_value;
    }
    if (in.equipment_30)
    {
        value *= 3;
    }
    value = value * p.random_multiplier / 10;
    if (!p.multiplayer && !p.player_side)
    {
        value = value * (100 + p.difficulty_bonus) / 100;
    }
    if (in.minimum_required) { *in.minimum_required = value < 1; }
    if (value < 1)
    {
        int minimumDamage;
        if (in.minimum_roll < 1) { minimumDamage = 5; }
        else if (in.minimum_roll < 6) { minimumDamage = 4; }
        else if (in.minimum_roll < 16) { minimumDamage = 3; }
        else if (in.minimum_roll < 46) { minimumDamage = 2; }
        else
        {
            minimumDamage = 1;
        }

        value = minimumDamage;
    }
    return Result<int>::success(value);
}

auto counter_damage(const CounterDamageInput& in) -> int
{
    auto result = try_counter_damage(in);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto hit_chance(const HitChanceInput& in) -> int
{
    int mode_bonus;
    if (in.attack_mode == 3) { mode_bonus = 5; }
    else if (in.attack_mode == 2) { mode_bonus = 3; }
    else
    {
        mode_bonus = 0;
    }

    if (in.counter_preview || in.counter)
    {
        mode_bonus = -mode_bonus;
    }
    int tactics = (in.preview || in.counter) ? in.attacker_tactics : adjusted_tactics(in.attacker_tactics, in.attacker_condition, in.condition_roll);
    auto modifiers = in.modifiers;
    modifiers.chance = (in.height_difference * 1000 + mode_bonus * 800 + 8000 + in.context_bonus) * tactics / 8000 - in.defender_evasion;
    int value = modify_hit_chance(modifiers);
    if (in.preview)
    {
        if (in.counter_preview)
        {
            if (in.counter_accuracy_penalty)
            {
                value /= 10;
            }
            value = value * 45 / 100;
        }
    }
    else
    {
        if (in.counter && in.counter_accuracy_penalty)
        {
            value /= 10;
        }
        value = value * 12 / 10;
        if (in.counter)
        {
            value = value * (in.counter_reduction_roll + 3) / 10;
        }
    }
    value = value * in.random_multiplier / 10;
    if (!in.multiplayer && !in.player_side)
    {
        value = value * (100 + in.difficulty_bonus) / 100;
    }
    value = std::clamp(value, 1, 99);
    if (!in.counter_preview && !in.counter && ((in.defender_39 & 2) || (in.defender_status3c_dword & 0x2A000)))
    {
        value = 99;
    }
    if (in.preview && (in.preview_immune || (in.defender_3d & 8)))
    {
        value = 0;
    }
    return value;
}

auto hit_succeeds(const HitChanceInput& in, int percent_roll) -> bool
{
    return percent_roll <= hit_chance(in) && !(in.defender_3d & 8);
}

auto consecutive_chance(const ConsecutiveInput& in) -> int
{
    if (!in.has_attack_context || (in.defender_115 & 4) || (in.opposing_mode & 2) || (in.defender_3f & 4) || in.defender_equipment_13)
    {
        return 0;
    }
    int value = in.attacker_tactics + in.attacker_equipment_bonus + in.mode_adjustment - in.defender_evasion - in.defender_equipment_penalty;
    if (in.attacker_115 & 1)
    {
        value += 30;
    }
    if (in.defender_115 & 2)
    {
        value -= 30;
    }
    if ((in.defender_3c & 4) || in.defender_equipment_16)
    {
        value -= value / 10;
    }
    return std::clamp(value, 1, 50);
}

auto consecutive_mode_adjustment(int attack_mode, bool reverse_direction) -> int
{
    int adjustment;
    if (attack_mode == 3) { adjustment = 10; }
    else if (attack_mode == 2) { adjustment = 5; }
    else
    {
        adjustment = 0;
    }

    return reverse_direction ? -adjustment : adjustment;
}

auto consecutive_succeeds(const ConsecutiveInput& in, int percent_roll) -> bool
{
    return percent_roll < consecutive_chance(in);
}

auto terrain_height_difference(unsigned attacker_height, unsigned defender_height, bool simple_rules) -> int
{
    return simple_rules ? 0 : int(attacker_height & 0xff) - int(defender_height & 0xff);
}

auto percent_at_most(const RandomDraws& r, int threshold) -> bool
{
    return r.percent_random_0_99 <= threshold;
}

auto within_decoded_range(const RangeInput& in) -> bool
{
    if (in.distance < 0 || in.range < 0)
    {
        return false;
    }
    return in.distance <= in.range;
}
}    //namespace s2ne
