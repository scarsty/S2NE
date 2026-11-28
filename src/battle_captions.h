#pragma once
#include "events.h"
#include "menu.h"

namespace s2ne
{
// 43AE24 packs 4EAC38/4EAC70 alongside the enabled CEL entries.
// 43A62D paints the selected entry's caption, not overlapping captions for every icon.
struct BattleCaptions
{
    static constexpr std::array<unsigned, 6> command_ids{ 54, 55, 56, 57, 198, 58 };
    static constexpr std::array<unsigned, 8> attack_ids{ 60, 61, 62, 63, 64, 65, 66, 67 };
    static std::vector<uint8_t> command(const Menu& font, int index, bool exhausted = false);
    static std::vector<uint8_t> attack(const Menu& font, int index, const Events::Unit& unit,
        const std::optional<Events::ActionRecord>& action,
        const std::vector<uint8_t>& items);
    static int left(const std::vector<uint8_t>& text, int icon_x);
    static void draw(const Menu& font, std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int icon_x, int icon_y);
};
}    //namespace s2ne
