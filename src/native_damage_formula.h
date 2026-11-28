#pragma once
#include "native_damage_records.h"
#include "resource_result.h"
namespace s2ne
{
// 46E858 arithmetic only. This does not authorize targeting, settle HP, or pay costs.
class NativeDamageFormula
{
    std::array<int, 256> config_{};
    static int multiply(int a, int b);
    static Result<int> divide(int value, int divisor);

public:
    struct Input
    {
        // proficiency is the original unsigned WORD, before the /10 at 46EA64.
        int actor_value{}, target_value{}, proficiency{}, target_defense{}, actor_hp{}, target_hp{}, target_attribute{}, actor_combat_value{};
    };
    static Result<int> evaluate(unsigned formula, int power, const Input& in, const std::array<int, 256>& config);
    static Result<NativeDamageFormula> create(const std::filesystem::path& root);
    Result<int> raw(unsigned id, const Input& in) const;
};
}    //namespace s2ne
