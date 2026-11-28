#pragma once
#include "combat.h"
#include "equipment.h"
#include "promotion_transaction.h"
#include "resource_result.h"
#include "save_bridge.h"
namespace s2ne
{
class ItemUse
{
    std::vector<uint8_t> items_;
    Equipment equipment_;
    std::filesystem::path root_;
    static int sw(const auto& b, size_t p) { return int(int16_t(word(b, p))); }
    static void put(auto& b, size_t p, int value)
    {
        b.at(p) = uint8_t(value);
        b.at(p + 1) = uint8_t(unsigned(value) >> 8);
    }
    static void add(auto& b, size_t p, int value) { put(b, p, sw(b, p) + value); }
    size_t offset(unsigned id) const;

public:
    explicit ItemUse(const std::filesystem::path& root);
    std::vector<unsigned> choices(const Save::State& state, size_t role) const;
    bool eligible(const Save::State& state, size_t role, unsigned id) const;
    bool usable_in_inn(unsigned id) const;
    unsigned spell(unsigned id) const;
    bool targeted(unsigned id) const;
    unsigned effect(unsigned id) const;
    int feedback_amount(const Save::State& before, const Save::State& after, size_t target, unsigned id,
        s2ne::Random random, uint32_t seconds) const;
    static const Save::UnitRecord* target_record(const Save::State& state, size_t target);
    bool target_allowed(const Save::State& state, size_t actor, size_t target, unsigned id) const;
    Result<bool> try_apply(Save::State& state, size_t role, unsigned id, s2ne::Random& random, uint32_t seconds,
        std::optional<size_t> target = std::nullopt) const;
    Result<Save::State> try_prepare(const Save::State& before, size_t role, unsigned id,
        s2ne::Random& random, uint32_t seconds, std::optional<size_t> target = std::nullopt) const;
};
}    //namespace s2ne
