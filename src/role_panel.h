#pragma once
#include "map.h"
#include "shop_menu.h"
namespace s2ne
{
// 44D675's217-panel: ten original rows, selected role portrait and native standing bitmap.
class RolePanel
{
    Menu font_;
    ShopMenu graphics_;
    mutable Map map_;
    std::vector<std::vector<uint8_t>> skill_names_;

public:
    explicit RolePanel(const std::filesystem::path& root, Disc disc = Disc::one);
    int top() const;
    void draw(std::vector<uint8_t>& image, const Events& events, std::span<const size_t> roles, size_t first, size_t row, std::optional<int> origin = std::nullopt) const;
};
}    //namespace s2ne
