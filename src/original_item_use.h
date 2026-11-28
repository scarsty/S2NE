#pragma once
#include "original_combat.h"
#include "original_equipment.h"
#include "original_promotion_transaction.h"
#include "original_save_bridge.h"
#include "resource_result.h"
namespace s2ne
{
class OriginalItemUse
{
    std::vector<uint8_t> items_;
    OriginalEquipment equipment_;
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
    explicit OriginalItemUse(const std::filesystem::path& root);
    std::vector<unsigned> choices(const OriginalSave::State& state, size_t role) const;
    bool eligible(const OriginalSave::State& state, size_t role, unsigned id) const;
    bool usable_in_inn(unsigned id) const;
    unsigned spell(unsigned id) const;
    bool targeted(unsigned id) const;
    unsigned effect(unsigned id) const;
    int feedback_amount(const OriginalSave::State& before, const OriginalSave::State& after, size_t target, unsigned id,
        s2ne::Random random, uint32_t seconds) const;
    static const OriginalSave::UnitRecord* target_record(const OriginalSave::State& state, size_t target);
    bool target_allowed(const OriginalSave::State& state, size_t actor, size_t target, unsigned id) const;
    Result<bool> try_apply(OriginalSave::State& state, size_t role, unsigned id, s2ne::Random& random, uint32_t seconds,
        std::optional<size_t> target = std::nullopt) const;
    Result<OriginalSave::State> try_prepare(const OriginalSave::State& before, size_t role, unsigned id,
        s2ne::Random& random, uint32_t seconds, std::optional<size_t> target = std::nullopt) const;
};
}    //namespace s2ne
