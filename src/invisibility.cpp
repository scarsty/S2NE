#include "invisibility.h"

namespace s2ne
{
auto Invisibility::shadow(std::span<uint8_t> pixels, std::span<const uint8_t> background, unsigned width, unsigned height,
    int displacement, std::span<const uint8_t> foot, const Palette& palette, bool special, bool movement9) -> Result<bool>
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

auto Invisibility::compose(std::span<uint8_t> pixels, std::span<const uint8_t> background, const Events::Unit& unit, const Palette& palette) -> Result<bool>
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
}    //namespace s2ne
