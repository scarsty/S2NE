#include "equipment.h"

namespace s2ne
{
auto Equipment::try_item(unsigned id) const -> s2ne::Result<size_t>
{
    if (!id || id > word(items_, 0))
    {
        return s2ne::Result<size_t>::failure(make_error("Equipment item ID"));
    }
    return s2ne::Result<size_t>::success(2 + size_t(id - 1) * 38);
}

auto Equipment::item(unsigned id) const -> size_t
{
    auto result = try_item(id);
    return std::move(result).value();
}

auto Equipment::flags(Save::State& state) const -> void
{
    auto& flag = state.groups[0].metadata.at(4);
    flag &= 0x81;
    for (const auto& r : state.groups[0].units)
    {
        if (r.at(0x33) & 0x30)
        {
            continue;
        }
        if (r.at(0x115) & 8)
        {
            flag |= 2;
        }
        if (r.at(0x117) & 2)
        {
            flag |= 16;
        }
        if (r.at(0x118) & 4)
        {
            flag |= 12;
        }
        if (sw(r, 8) != 1)
        {
            if (r.at(0x117) & 4)
            {
                flag |= 32;
            }
            if (r.at(0x118) & 128)
            {
                flag |= 64;
            }
        }
        for (unsigned i = 0; i < 6; ++i)
        {
            if (auto id = word(r, 0xCB + 2 * i))
            {
                size_t p = item(id);
                if (!items_[p + 15] && int8_t(items_[p + 25]) > 0)
                {
                    if (items_[p + 25] == 31)
                    {
                        flag |= 8;
                    }
                    if (items_[p + 25] == 58)
                    {
                        flag |= 4;
                    }
                }
            }
        }
    }
}

auto Equipment::unequip_slot(Save::State& state, size_t index, unsigned s) const -> void
{
    auto& army = state.groups[0];
    auto& r = army.units.at(index);
    unsigned old = word(r, 0xCB + 2 * s);
    if (old)
    {
        army.inventory.at(old - 1) = uint8_t(std::min(99u, unsigned(army.inventory.at(old - 1)) + 1));
        size_t p = item(old);
        auto remove = [&](int d, size_t delta, size_t current, size_t currentZero, size_t limit, size_t limitZero)
        {
            if (!d)
            {
                return;
            }
            add(r, delta, -d);
            add(r, current, -d);
            if (sw(r, current) < 0)
            {
                put(r, currentZero, 0);
            }
            add(r, limit, -d);
            if (sw(r, limit) < 0)
            {
                put(r, limitZero, 0);
            }
        };
        remove(sw(items_, p + 17), 0x13A, 0x134, 0x134, 0xC3, 0xC3);
        remove(int8_t(items_[p + 21]), 0x13C, 0x136, 0x136, 0xC5, 0xC5);
        remove(int8_t(items_[p + 22]), 0x13E, 0x12A, 0x136, 0xBD, 0xC5);
        int d = int8_t(items_[p + 24]);
        if (d)
        {
            add(r, 0x140, -d);
            add(r, 0x12C, -d);
            if (sw(r, 0x12C) < 0)
            {
                put(r, 0x12C, 0);
            }
            add(r, 0x12E, -d);
            if (sw(r, 0x12E) < 0)
            {
                put(r, 0x12E, 0);
            }
            add(r, 0xBF, -d);
            if (sw(r, 0xBF) < 0)
            {
                put(r, 0xBF, 0);
            }
        }
    }
    put(r, 0xCB + 2 * s, 0);
}

Equipment::Equipment(const std::filesystem::path& root)
{
    auto read = [](const auto& p)
    {
        return read_resource(p).value();
    };
    items_ = read(root / "ITEMSP.BIN");
    kinds_ = read(root / "KINDSP.BIN");
}

auto Equipment::try_auto_equip(Save::State& state) const -> s2ne::Result<bool>
{
    auto staged = state;
    for (size_t index = 0; index < staged.groups[0].units.size(); ++index)
    {
        const auto& role = staged.groups[0].units[index];
        if ((role.at(0x33) & 0x70) || sw(role, 8) == 1)
        {
            continue;
        }
        for (unsigned s = 0; s < 6; ++s)
        {
            unsigned best = 0, value = 0;
            for (unsigned id = 1; id <= word(items_, 0); ++id)
            {
                unsigned t = type(id);
                if (!staged.groups[0].inventory.at(id - 1) || !(s ? t == s + 6 : t >= 1 && t <= 6 && weapon_skill(staged.groups[0].units[index], id)) || !eligible(staged.groups[0].units[index], id))
                {
                    continue;
                }
                unsigned rank = items_.at(item(id) + 37);
                if (rank > value)
                {
                    best = id;
                    value = rank;
                }
            }
            unsigned old = word(staged.groups[0].units[index], 0xCB + 2 * s);
            if (old && eligible(staged.groups[0].units[index], old) && (s || weapon_skill(staged.groups[0].units[index], old)) && items_.at(item(old) + 37) > value)
            {
                continue;
            }
            unequip_slot(staged, index, s);
            if (best && !equip(staged, index, best))
            {
                return s2ne::Result<bool>::failure(make_error("451B21 original auto equipment rejected"));
            }
        }
        auto& r = staged.groups[0].units[index];
        unsigned weapon = word(r, 0xCB);
        if (word(r, 0xD3) && weapon && type(weapon) == 4)
        {
            unequip_slot(staged, index, 4);
        }
    }
    flags(staged);
    state = std::move(staged);

    return s2ne::Result<bool>::success(true);
}

auto Equipment::auto_equip(Save::State& state) const -> void
{
    auto result = try_auto_equip(state);
    if (!result)
    {
        report_error(result.error());
    }
}

auto Equipment::type(unsigned id) const -> unsigned { return items_.at(item(id) + 14); }

auto Equipment::try_slot(unsigned id) const -> s2ne::Result<unsigned>
{
    unsigned t = type(id);
    if (t >= 1 && t <= 6)
    {
        return s2ne::Result<unsigned>::success(0);
    }
    if (t >= 7 && t <= 11)
    {
        return s2ne::Result<unsigned>::success(t - 6);
    }
    return s2ne::Result<unsigned>::failure(make_error("4125FC non equipment"));
}

auto Equipment::slot(unsigned id) const -> unsigned
{
    auto result = try_slot(id);
    return std::move(result).value();
}

auto Equipment::field(unsigned id, unsigned offset, bool wide) const -> int
{
    if (!id) { return 0; }
    else if (wide) { return sw(items_, item(id) + offset); }
    else
    {
        return int(int8_t(items_.at(item(id) + offset)));
    }
}

auto Equipment::recipients(const Save::State& state, unsigned id) const -> std::vector<size_t>
{
    std::vector<size_t> out;
    for (size_t i = 0; i < state.groups[0].units.size(); ++i)
    {
        const auto& r = state.groups[0].units[i];
        const bool available = state.battle_context() ? !(r.at(0x33) & 0x60) : !(r.at(0x33) & 0x70) && sw(r, 8) != 1;
        if (available && eligible(r, id))
        {
            out.push_back(i);
        }
    }
    return out;
}

auto Equipment::try_unequip(Save::State& state, size_t index, unsigned destination) const -> Result<bool>
{
    if (index >= state.groups[0].units.size() || destination >= 6 || state.groups[0].inventory.size() != word(items_, 0))
    {
        return Result<bool>::failure(make_error("Invalid original44D11B equipment removal"));
    }
    auto& role = state.groups[0].units[index];
    unsigned old = word(role, 0xCB + destination * 2);
    const bool unavailable = state.battle_context() ? (role[0x33] & 0x60) != 0 : (role[0x33] & 0x70) != 0 || sw(role, 8) == 1;
    if (unavailable || old > word(items_, 0))
    {
        return Result<bool>::failure(make_error("Original44D439 equipment recipient unavailable"));
    }
    auto staged = state;
    unequip_slot(staged, index, destination);
    flags(staged);
    state = std::move(staged);
    return Result<bool>::success(true);
}

auto Equipment::equip(Save::State& state, size_t index, unsigned id) const -> bool
{
    if (!id || id > word(items_, 0) || index >= state.groups[0].units.size() || state.groups[0].inventory.size() != word(items_, 0))
    {
        return false;
    }
    auto destination = try_slot(id);
    if (!destination)
    {
        report_error(destination.error());
        return false;
    }
    auto allowed = recipients(state, id);
    if (std::find(allowed.begin(), allowed.end(), index) == allowed.end() || !state.groups[0].inventory.at(id - 1))
    {
        return false;
    }
    auto copy = state;
    auto& army = copy.groups[0];
    auto& r = army.units[index];
    unsigned s = destination.value();
    unequip_slot(copy, index, s);
    put(r, 0xCB + 2 * s, id);
    --army.inventory[id - 1];
    size_t p = item(id);
    std::array<int, 4> values{ sw(items_, p + 17), int(int8_t(items_[p + 21])), int(int8_t(items_[p + 22])), int(int8_t(items_[p + 24])) };
    std::array<size_t, 4> current{ 0x134, 0x136, 0x12A, 0x12C }, limit{ 0xC3, 0xC5, 0xBD, 0xBF };
    for (unsigned i = 0; i < 4; ++i)
    {
        int d = values[i], cap = i == 3 ? 110 : 999;
        if (sw(r, limit[i]) + d > cap)
        {
            d = 999 - sw(r, limit[i]);
        }
        add(r, 0x13A + 2 * i, d);
        add(r, current[i], d);
        if (i == 3)
        {
            if (sw(r, 0x12C) < 0)
            {
                put(r, 0x12C, 0);
            }
            add(r, 0x12E, d);
            if (sw(r, 0x12E) < 0)
            {
                put(r, 0x12E, 0);
            }
        }
        add(r, limit[i], d);
    }
    flags(copy);
    state = std::move(copy);
    return true;
}
}    //namespace s2ne
