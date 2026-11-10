#pragma once
#include "original_shop.hpp"
namespace s2ne
{
class OriginalEquipment
{
    std::vector<uint8_t> items_, kinds_;
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static int sw(const auto& b, size_t p) { return int(int16_t(word(b, p))); }
    static void put(auto& b, size_t p, int v)
    {
        b.at(p) = uint8_t(v);
        b.at(p + 1) = uint8_t(unsigned(v) >> 8);
    }
    static void add(auto& b, size_t p, int v) { put(b, p, sw(b, p) + v); }
    size_t item(unsigned id) const
    {
        if (!id || id > word(items_, 0))
        {
            throw std::runtime_error("Equipment item ID");
        }
        return 2 + size_t(id - 1) * 38;
    }
    bool special(const auto& r, unsigned code) const
    {
        for (unsigned i = 0; i < 6; ++i)
        {
            if (auto id = word(r, 0xCB + 2 * i); id && !items_.at(item(id) + 15) && items_.at(item(id) + 25) == code)
            {
                return true;
            }
        }
        return false;
    }
    void flags(OriginalSave::State& state) const
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
    void unequip_slot(OriginalSave::State& state, size_t index, unsigned s) const
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

public:
    explicit OriginalEquipment(const std::filesystem::path& root)
    {
        auto read = [](const auto& p)
        {
            std::ifstream f(p, std::ios::binary);
            if (!f)
            {
                throw std::runtime_error("Missing original equipment data");
            }
            return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), { });
        };
        items_ = read(root / "ITEMSP.BIN");
        kinds_ = read(root / "KINDSP.BIN");
    }
    bool weapon_skill(const auto& r, unsigned id) const
    {
        size_t table = word(kinds_, 30) + (size_t(word(kinds_, 32)) << 16), kind = word(r, 4), count = word(kinds_, 22);
        if (!kind || kind > count)
        {
            throw std::runtime_error("440C06 kind index");
        }
        size_t entry = table + (kind - 1) * 4, first = word(kinds_, entry), n = word(kinds_, entry + 2), records = table + count * 4;
        for (size_t i = 0; i < n; ++i)
        {
            if (int8_t(kinds_.at(records + (first + i) * 23)) == int(type(id) + 1))
            {
                return true;
            }
        }
        return false;
    }
    void auto_equip(OriginalSave::State& state) const
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
                    throw std::runtime_error("451B21 original auto equipment rejected");
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
    }
    unsigned type(unsigned id) const { return items_.at(item(id) + 14); }
    unsigned slot(unsigned id) const
    {
        unsigned t = type(id);
        if (t >= 1 && t <= 6)
        {
            return 0;
        }
        if (t >= 7 && t <= 11)
        {
            return t - 6;
        }
        throw std::runtime_error("4125FC non equipment");
    }
    int field(unsigned id, unsigned offset, bool wide = false) const { return !id ? 0 : wide ? sw(items_, item(id) + offset) :
                                                                                               int(int8_t(items_.at(item(id) + offset))); }
    bool eligible(const auto& r, unsigned id) const
    {
        size_t p = item(id);
        unsigned kind = word(r, 4), identity = word(r, 2);
        if ((items_[p + 35] & 64) && identity != 11)
        {
            return false;
        }
        if ((items_[p + 35] & 128) && identity != 22)
        {
            return false;
        }
        if ((items_[p + 36] & 1) && identity != 1)
        {
            return false;
        }
        if (!(r.at(0x118) & 8) || type(id) <= 6)
        {
            size_t k = word(kinds_, 18) + (size_t(word(kinds_, 20)) << 16) + (kind - 1) * 37;
            unsigned category = kinds_.at(k + 17);
            uint32_t mask = 0;
            for (unsigned i = 0; i < 4; ++i)
            {
                mask |= uint32_t(items_.at(p + 31 + i)) << (8 * i);
            }
            if (!(mask & (1u << ((category - 1) & 31))))
            {
                return false;
            }
        }
        unsigned attribute = r.at(0x123) + (special(r, 64) ? 10 : 0), moral = attribute <= 30 ? 1 : attribute < 70 ? 2 :
                                                                                                                     4,
                 gender = r.at(0x2E) == 0 ? 8 : r.at(0x2E) == 1 ? 16 :
                                                                  32,
                 allowed = word(items_, p + 35);
        return (allowed & moral) && ((r.at(0x118) & 8) || (allowed & gender));
    }
    std::vector<size_t> recipients(const OriginalSave::State& state, unsigned id) const
    {
        std::vector<size_t> out;
        for (size_t i = 0; i < state.groups[0].units.size(); ++i)
        {
            const auto& r = state.groups[0].units[i];
            if (!(r.at(0x33) & 0x70) && sw(r, 8) != 1 && eligible(r, id))
            {
                out.push_back(i);
            }
        }
        return out;
    }
    bool equip(OriginalSave::State& state, size_t index, unsigned id) const
    {
        auto allowed = recipients(state, id);
        if (std::find(allowed.begin(), allowed.end(), index) == allowed.end() || !state.groups[0].inventory.at(id - 1))
        {
            return false;
        }
        auto copy = state;
        auto& army = copy.groups[0];
        auto& r = army.units[index];
        unsigned s = slot(id);
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
};
}    //namespace s2ne
