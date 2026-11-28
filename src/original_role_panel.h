#pragma once
#include "original_map.h"
#include "original_shop_menu.h"
namespace s2ne
{
// 44D675's217-panel: ten original rows, selected role portrait and native standing bitmap.
class OriginalRolePanel
{
    OriginalMenu font_;
    OriginalShopMenu graphics_;
    mutable OriginalMap map_;
    std::vector<std::vector<uint8_t>> skill_names_;

public:
    explicit OriginalRolePanel(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one);
    int top() const;
    void draw(std::vector<uint8_t>& image, const OriginalEvents& events, std::span<const size_t> roles, size_t first, size_t row, std::optional<int> origin = std::nullopt) const;
};
}    //namespace s2ne
