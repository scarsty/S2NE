#pragma once
#include "resource_result.h"
#include "shop.h"
namespace s2ne
{
class Equipment
{
    std::vector<uint8_t> items_, kinds_;
    static int sw(const auto& b, size_t p) { return int(int16_t(word(b, p))); }
    static void put(auto& b, size_t p, int v)
    {
        b.at(p) = uint8_t(v);
        b.at(p + 1) = uint8_t(unsigned(v) >> 8);
    }
    static void add(auto& b, size_t p, int v) { put(b, p, sw(b, p) + v); }
    s2ne::Result<size_t> try_item(unsigned id) const;
    size_t item(unsigned id) const;
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
    void flags(Save::State& state) const;
    void unequip_slot(Save::State& state, size_t index, unsigned s) const;

public:
    explicit Equipment(const std::filesystem::path& root);
    s2ne::Result<bool> try_weapon_skill(const auto& r, unsigned id) const
    {
        size_t table = word(kinds_, 30) + (size_t(word(kinds_, 32)) << 16), kind = word(r, 4), count = word(kinds_, 22);
        if (!kind || kind > count)
        {
            return s2ne::Result<bool>::failure(make_error("440C06 kind index"));
        }
        size_t entry = table + (kind - 1) * 4, first = word(kinds_, entry), n = word(kinds_, entry + 2), records = table + count * 4;
        for (size_t i = 0; i < n; ++i)
        {
            if (int8_t(kinds_.at(records + (first + i) * 23)) == int(type(id) + 1))
            {
                return s2ne::Result<bool>::success(true);
            }
        }
        return s2ne::Result<bool>::success(false);
    }
    bool weapon_skill(const auto& r, unsigned id) const
    {
        auto result = try_weapon_skill(r, id);
        if (!result)
        {
            report_error(result.error());
            return false;
        }
        return result.value();
    }
    s2ne::Result<bool> try_auto_equip(Save::State& state) const;
    void auto_equip(Save::State& state) const;
    unsigned type(unsigned id) const;
    s2ne::Result<unsigned> try_slot(unsigned id) const;
    unsigned slot(unsigned id) const;
    int field(unsigned id, unsigned offset, bool wide = false) const;
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
        unsigned attribute = r.at(0x123) + (special(r, 64) ? 10 : 0);
        int moralMask;
        if (attribute <= 30) { moralMask = 1; }
        else if (attribute < 70) { moralMask = 2; }
        else
        {
            moralMask = 4;
        }
        int genderMask;
        if (r.at(0x2E) == 0) { genderMask = 8; }
        else if (r.at(0x2E) == 1) { genderMask = 16; }
        else
        {
            genderMask = 32;
        }

        unsigned moral = moralMask,
                 gender = genderMask,
                 allowed = word(items_, p + 35);
        return (allowed & moral) && ((r.at(0x118) & 8) || (allowed & gender));
    }
    std::vector<size_t> recipients(const Save::State& state, unsigned id) const;
    Result<bool> try_unequip(Save::State& state, size_t index, unsigned destination) const;
    bool equip(Save::State& state, size_t index, unsigned id) const;
};
}    //namespace s2ne
