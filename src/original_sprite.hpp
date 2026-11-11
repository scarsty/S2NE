#pragma once
#include "resource_result.hpp"
#include <algorithm>

namespace s2ne
{
// 474E3E arg_0 applies only to zero opcodes/repeats. Literals copy even index 0.
enum class OriginalZeroRun
{
    transparent,
    opaque
};
inline Result<bool> draw_original_sprite(std::vector<uint8_t>& image, std::span<const uint8_t> bytes,
    std::span<const uint8_t> palette, size_t position, bool typed, int left, int top, OriginalZeroRun zero)
{
    if (image.size() != 640u * 480 * 4 || palette.size() < 768)
    {
        return Result<bool>::failure(make_error("Invalid original sprite framebuffer/palette"));
    }
    ResourceReader reader(bytes);
    unsigned type = typed ? reader.number(position++, 1) : 0;
    unsigned width = reader.number(position) + 1, height = reader.number(position + 2) + 1;
    position += 4;
    if (!reader || width > 640 || height > 480 || type > 1)
    {
        return Result<bool>::failure(make_error("Invalid original sprite header"));
    }
    auto output = image;
    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width;)
        {
            unsigned op = type ? 0x81 : reader.number(position++, 1);
            unsigned count = op ? op & 127 : 1;
            if (!reader || !count || count > width - x)
            {
                return Result<bool>::failure(make_error("Invalid original sprite run"));
            }
            unsigned repeat = op && !(op & 128) ? reader.number(position++, 1) : 0;
            while (count--)
            {
                unsigned ink = op & 128 ? reader.number(position++, 1) : repeat;
                if (!reader)
                {
                    return Result<bool>::failure(reader.error());
                }
                int dx = left + int(x++), dy = top + int(y);
                // Keep the existing raw typed-image index mask distinct from RLE literals.
                bool skip = !ink && (type || (!(op & 128) && zero == OriginalZeroRun::transparent));
                if (skip || dx < 0 || dx >= 640 || dy < 0 || dy >= 480)
                {
                    continue;
                }
                size_t dst = (size_t(dy) * 640 + dx) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    output[dst + k] = uint8_t(std::min(255u, unsigned(palette[ink * 3 + k]) * 255 / 63));
                }
                output[dst + 3] = 255;
            }
        }
    }
    image.swap(output);
    return Result<bool>::success(true);
}
inline Result<bool> draw_original_cel(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    std::span<const uint8_t> palette, unsigned id, int left, int top, OriginalZeroRun zero)
{
    ResourceReader pal(palette), data(cel);
    unsigned address = pal.number(816, 4);
    if (!pal || address < 57 || id < 142)
    {
        return Result<bool>::failure(make_error("Invalid original CEL directory"));
    }
    size_t group = address - 57;
    unsigned count = data.number(group);
    if (id - 142 >= count)
    {
        return Result<bool>::failure(make_error("Invalid original CEL ID"));
    }
    size_t directory = group + 2 + size_t(id - 142) * 8;
    size_t offset = data.number(directory, 4), length = data.number(directory + 4, 4);
    if (!data || !data.extent(group + offset, length) || length < 4)
    {
        return Result<bool>::failure(make_error("Invalid original CEL extent"));
    }
    return draw_original_sprite(image, cel.subspan(group + offset, length), palette, 0, false, left, top, zero);
}
}    //namespace s2ne
