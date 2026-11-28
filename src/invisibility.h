#pragma once
#include "events.h"
#include "palette.h"
namespace s2ne
{
struct Invisibility
{
    static Result<bool> shadow(std::span<uint8_t> pixels, std::span<const uint8_t> background, unsigned width, unsigned height,
        int displacement, std::span<const uint8_t> foot, const Palette& palette, bool special, bool movement9);
    template <class Elevation>
    static bool hidden(const Events::Unit& unit, const std::vector<Events::Unit>& roster, unsigned turn, unsigned opposing_mode, Elevation elevation)
    {
        (void)turn;    // 446E53 tests army membership and flags, not HP or deployment dates.
        if (!(unit.abilities & 0x80) && !(unit.status3c & 0x10)) { return false; }
        if (opposing_mode & 9) { return false; }
        const int right = unit.x + int(unit.y & 1), left = right - 1;
        const int height = elevation(unit.x, unit.y);
        for (auto [x, y] : std::array<std::pair<int, int>, 4>{ { { right, unit.y - 1 }, { left, unit.y + 1 }, { left, unit.y - 1 }, { right, unit.y + 1 } } })
        {
            for (const auto& observer : roster)
            {
                if (observer.hostile() == unit.hostile() || !(observer.status_flags & 0x80) || ((observer.status_flags & 0x20) && !(observer.status_flags & 2)) || observer.x != x || observer.y != y) { continue; }
                const int other = elevation(x, y);
                if (other > 0 && other - height >= -3 && other - height <= 2) { return false; }
                break;
            }
        }
        return true;
    }
    // 442309 single-player: friendly active=shadowed B1, spent=32 silhouette;
    // enemy=unmodified B1. Source zero pixels remain transparent.
    static Result<bool> compose(std::span<uint8_t> pixels, std::span<const uint8_t> background, const Events::Unit& unit, const Palette& palette);
};
}    //namespace s2ne
