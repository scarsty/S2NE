#pragma once
#include "resource_result.h"
#include <array>
namespace s2ne
{
Result<std::vector<uint8_t>> read_spell_entry(const std::filesystem::path& root, unsigned skill, unsigned visual, unsigned color, unsigned amount, unsigned dispatch = 0, unsigned cursor = 0);
}    //namespace s2ne
