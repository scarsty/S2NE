#pragma once
#include "original_progression.h"
#include "original_shop_menu.h"
namespace s2ne
{
class OriginalGrowthPanel
{
public:
    static Result<std::vector<uint8_t>> draw(const std::filesystem::path& root, std::vector<uint8_t> image, const OriginalProgression::GrowthStep& step);
};
}    // namespace s2ne
