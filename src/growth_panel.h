#pragma once
#include "progression.h"
#include "shop_menu.h"
namespace s2ne
{
class GrowthPanel
{
public:
    static Result<std::vector<uint8_t>> draw(const std::filesystem::path& root, std::vector<uint8_t> image, const Progression::GrowthStep& step);
};
}    // namespace s2ne
