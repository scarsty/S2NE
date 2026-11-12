#pragma once
#include "original_events.hpp"
#include "original_palette.hpp"
namespace s2ne
{
struct NativeInvisibility
{
    static Result<bool> shadow(std::span<uint8_t> pixels, std::span<const uint8_t> background, unsigned width, unsigned height,
        int displacement, std::span<const uint8_t> foot, const OriginalPalette& palette, bool special, bool movement9)
    {
        if (!width || height < 32 || pixels.size() != size_t(width) * height || background.size() != pixels.size() || foot.size() != 2048)
        {
            return Result<bool>::failure(make_error("448984 AD/B1/foot extent mismatch"));
        }
        if (special && movement9) { return Result<bool>::success(false); }
        int destination = int((height - 32) * width + width / 2) - 32 - displacement;
        const int source = int((height - 32) * width + width / 2) - 32;
        if (special)
        {
            if (int((height - 32) * width) + displacement <= 0) { return Result<bool>::success(false); }
            destination = int((height - 32) * width + width / 2) - 28 + displacement;
        }
        else
        {
            destination += 4;
        }
        for (int y = 0; y < 32; ++y)
        {
            for (int x = 0; x < 64; ++x)
            {
                const int p = destination + y * int(width) + x, q = source + y * int(width) + x;
                const auto mask = foot[size_t(y) * 64 + x];
                if (!mask || p < 0 || p >= int(pixels.size())) { continue; }
                if (special) { pixels[size_t(p)] = mask; }
                else if (!pixels[size_t(p)] && q >= 0 && q < int(background.size())) { pixels[size_t(p)] = palette.map(5, background[size_t(q)]); }
            }
        }
        return Result<bool>::success(true);
    }
    template <class Elevation>
    static bool hidden(const OriginalEvents::Unit& unit, const std::vector<OriginalEvents::Unit>& roster, unsigned turn, unsigned opposing_mode, Elevation elevation)
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
    static Result<bool> compose(std::span<uint8_t> pixels, std::span<const uint8_t> background, const OriginalEvents::Unit& unit, const OriginalPalette& palette)
    {
        if (pixels.size() != background.size()) { return Result<bool>::failure(make_error("Invisibility AD/B1 extent mismatch")); }
        for (size_t i = 0; i < pixels.size(); ++i)
        {
            if (!pixels[i]) { continue; }
            if (unit.hostile()) { pixels[i] = background[i]; }
            else if (unit.action_state == 2 && pixels[i] != 32) { pixels[i] = 32; }
            else
            {
                pixels[i] = palette.map(6, background[i]);
            }
        }
        return Result<bool>::success(true);
    }
};
}    //namespace s2ne
