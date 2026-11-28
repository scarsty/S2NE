#include "native_damage_formula.h"

namespace s2ne
{
auto NativeDamageFormula::multiply(int a, int b) -> int { return int32_t(uint32_t(a) * uint32_t(b)); }

auto NativeDamageFormula::divide(int value, int divisor) -> Result<int>
{
    if (!divisor || (value == INT32_MIN && divisor == -1))
    {
        return Result<int>::failure(make_error("Invalid original signed IDIV"));
    }
    return Result<int>::success(value / divisor);
}

auto NativeDamageFormula::evaluate(unsigned formula, int power, const Input& in, const std::array<int, 256>& config) -> Result<int>
{
    if (in.actor_value < 1 || in.target_value < 1 || in.proficiency < 0 || in.proficiency > 65535)
    {
        return Result<int>::failure(make_error("Invalid 46E858 effective arithmetic inputs"));
    }
    unsigned coefficient = 0;
    switch (formula)
    {
    case 5: coefficient = 0x1A; break;
    case 7: coefficient = 0x20; break;
    case 8: coefficient = 0x24; break;
    case 9: coefficient = 0x28; break;
    case 10: coefficient = 0x2C; break;
    case 12: coefficient = 0x34; break;
    case 15: coefficient = 0x3C; break;
    case 17: coefficient = 0x42; break;
    case 18: coefficient = 0x46; break;
    case 19: coefficient = 0x4A; break;
    case 20: coefficient = 0x4E; break;
    case 21: coefficient = 0x52; break;
    case 24: coefficient = 0x5A; break;
    case 26: coefficient = 0x60; break;
    case 30: coefficient = 0x6A; break;
    case 33: coefficient = 0x74; break;
    case 36: coefficient = 0x7A; break;
    case 37: coefficient = 0x7E; break;
    case 38: coefficient = 0x82; break;
    case 39: coefficient = 0x86; break;
    case 40: coefficient = 0x8A; break;
    case 42: coefficient = 0x92; break;
    default: break;
    }
    if (coefficient)
    {
        auto attack = divide(multiply(in.actor_value, config[coefficient]), config[coefficient + 1]);
        auto defense = divide(multiply(in.target_value, config[coefficient + 2]), config[coefficient + 3]);
        if (!attack) { return attack; }
        if (!defense) { return defense; }
        return Result<int>::success(int32_t(uint32_t(attack.value()) - uint32_t(defense.value()) + uint32_t(power)));
    }
    if (formula >= 44 && formula <= 64 && formula != 46)
    {
        coefficient = 0x98 + (formula - 44) * 2;
        const int attack = multiply(in.proficiency / 10 + 1000, in.actor_value) / 1000;
        auto defense = divide(multiply(in.target_value, config[coefficient]), config[coefficient + 1]);
        if (!defense) { return defense; }
        return Result<int>::success(int32_t(uint32_t(attack) - uint32_t(defense.value()) - uint32_t(in.target_defense) + uint32_t(power)));
    }
    int value = 0;
    switch (formula)
    {
    case 23:
        coefficient = 0x56;
        value = in.actor_hp;
        break;
    case 27:
        coefficient = 0x64;
        value = in.actor_combat_value;
        break;
    case 32:
        coefficient = 0x72;
        value = in.actor_value;
        break;
    case 34: return Result<int>::success(in.target_hp);
    case 35:
        value = multiply(in.actor_value, in.actor_value) / in.target_value;
        {
            auto scaled = divide(multiply(value, config[0x76]), config[0x77]);
            if (!scaled) { return scaled; }
            return Result<int>::success(int32_t(uint32_t(scaled.value()) + uint32_t(power)));
        }
    case 43:
        coefficient = 0x96;
        value = in.actor_hp;
        break;
    case 70:
        coefficient = 0xCC;
        value = in.target_attribute;
        break;
    case 71:
        coefficient = 0xCE;
        value = 100 - in.target_attribute;
        break;
    case 73:
        coefficient = 0xD2;
        value = in.target_hp;
        break;
    default: return Result<int>::failure(make_error("Unsupported original effect1 formula"));
    }
    auto scaled = divide(multiply(value, config[coefficient]), config[coefficient + 1]);
    if (!scaled) { return scaled; }
    return Result<int>::success(int32_t(uint32_t(scaled.value()) + uint32_t(power)));
}

auto NativeDamageFormula::create(const std::filesystem::path& root) -> Result<NativeDamageFormula>
{
    auto resources = read_combat_resources(root);
    if (!resources) { return Result<NativeDamageFormula>::failure(resources.error()); }
    ResourceReader pal(resources.value().pal), bin(resources.value().bin);
    const auto header = pal.number(856, 4) - 12;
    const auto first = header + 9 + 2 * bin.number(header + 1, 1);
    unsigned count = 0;
    for (unsigned id = 1; id <= 115; ++id)
    {
        const auto offset = first + (id - 1) * 37;
        if (bin.number(offset + 19, 1) != 1) { continue; }
        const auto* record = NativeDamageRecords::find(id);
        if (!record || !bin.extent(offset, 37) || !std::equal(record->raw.begin(), record->raw.end(), resources.value().bin.begin() + offset))
        {
            return Result<NativeDamageFormula>::failure(make_error("Original effect1 inventory mismatch"));
        }
        ++count;
    }
    if (!pal || !bin || count != NativeDamageRecords::records.size())
    {
        return Result<NativeDamageFormula>::failure(make_error("Incomplete original effect1 inventory"));
    }
    NativeDamageFormula result;
    for (unsigned i = 0; i < 256; ++i) { result.config_[i] = int(int8_t(resources.value().bin[resources.value().config + i])); }
    return Result<NativeDamageFormula>::success(result);
}

auto NativeDamageFormula::raw(unsigned id, const Input& in) const -> Result<int>
{
    const auto* record = NativeDamageRecords::find(id);
    if (!record) { return Result<int>::failure(make_error("Not an original effect1 record")); }
    const int power = int(int16_t(unsigned(record->raw[21]) | unsigned(record->raw[22]) << 8));
    return evaluate(record->raw[20], power, in, config_);
}
}    //namespace s2ne
