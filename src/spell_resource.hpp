#pragma once
#include "resource_result.hpp"
#include <array>
namespace s2ne
{
inline Result<std::vector<uint8_t>> read_spell_entry(const std::filesystem::path& root, unsigned skill, unsigned visual, unsigned color, unsigned amount)
{
    auto pal = read_resource(root / "SUPERSP.PAL");
    if (!pal)
    {
        return Result<std::vector<uint8_t>>::failure(pal.error());
    }
    auto bin = read_resource(root / "SUPERSP.BIN");
    if (!bin)
    {
        return Result<std::vector<uint8_t>>::failure(bin.error());
    }
    auto cel = read_resource(root / "SUPERSP.CEL");
    if (!cel)
    {
        return Result<std::vector<uint8_t>>::failure(cel.error());
    }
    ResourceReader p(pal.value()), b(bin.value()), c(cel.value());
    unsigned raw_table = p.number(920, 4), raw_base = p.number(808, 4);
    if (!p)
    {
        return Result<std::vector<uint8_t>>::failure(p.error());
    }
    if (raw_table < 20 || raw_base < 56)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid spell table address"));
    }
    size_t table = raw_table - 20, base = raw_base - 56;
    unsigned count = b.number(table);
    if (count != 115 || skill < 1 || skill > count || !b.extent(table + 2, size_t(count) * 4))
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid spell dispatch extent"));
    }
    if (b.number(table + 2 + skill - 1, 1) != visual || b.number(table + 2 + count + skill - 1, 1) != 0 || b.number(table + 2 + count * 2 + skill - 1, 1) != color || b.number(table + 2 + count * 3 + skill - 1, 1) != amount)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Unsupported original spell dispatch"));
    }
    if (c.number(base) < visual + 1)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Missing original spell entry"));
    }
    size_t offset = c.number(base + 2 + visual * 8, 4), length = c.number(base + 6 + visual * 8, 4);
    if (!c)
    {
        return Result<std::vector<uint8_t>>::failure(c.error());
    }
    if (base > cel.value().size() || offset > cel.value().size() - base || length > cel.value().size() - base - offset)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid spell entry extent"));
    }
    return Result<std::vector<uint8_t>>::success({ cel.value().begin() + base + offset, cel.value().begin() + base + offset + length });
}
}    //namespace s2ne
