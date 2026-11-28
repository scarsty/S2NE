#include "original_battle_captions.h"

namespace s2ne
{
auto OriginalBattleCaptions::command(const OriginalMenu& font, int index, bool exhausted) -> std::vector<uint8_t>
{
    return font.text_bytes(index == 5 && exhausted ? 59 : command_ids.at(index));
}

auto OriginalBattleCaptions::attack(const OriginalMenu& font, int index, const OriginalEvents::Unit& unit,
    const std::optional<OriginalEvents::ActionRecord>& action,
    const std::vector<uint8_t>& items) -> std::vector<uint8_t>
{
    // 43B135 uses weapon+2, otherwise action+1; 43B1D1 uses action+1.
    if (action && (index == 0 || index == 7))
    {
        std::vector<uint8_t> text;
        if (index == 0 && unit.equipment[0])
        {
            size_t p = 2 + size_t(unit.equipment[0] - 1) * 38 + 2;
            for (unsigned i = 0; i < 12 && items.at(p + i); ++i)
            {
                text.push_back(items.at(p + i));
            }
        }
        else
        {
            for (unsigned i = 1; i < 11 && action->fields[i]; ++i)
            {
                text.push_back(uint8_t(action->fields[i]));
            }
        }
        if (!text.empty())
        {
            return text;
        }
    }
    return font.text_bytes(attack_ids.at(index));
}

auto OriginalBattleCaptions::left(const std::vector<uint8_t>& text, int icon_x) -> int
{
    int x = icon_x + 16 - int(text.size()) * 4;
    if (x < 0)
    {
        return 0;
    }
    return std::min(x, 639 - int(text.size()) * 8);
}

auto OriginalBattleCaptions::draw(const OriginalMenu& font, std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int icon_x, int icon_y) -> void
{
    // 43A866/43AA0E -> 422402(ink=88h, shadow=9Ah, style=1).
    int x = left(text, icon_x), y = icon_y + 40;
    auto output = image;
    auto shadow = font.try_draw_small_validated(output, text, x + 1, y + 1, 154, true);
    if (!shadow)
    {
        report_error(shadow.error());
        return;
    }
    auto foreground = font.try_draw_small_validated(output, text, x, y, 136, true);
    if (!foreground)
    {
        report_error(foreground.error());
        return;
    }
    image.swap(output);
}
}    //namespace s2ne
