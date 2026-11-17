#pragma once
#include "resource_result.hpp"
#include "texture_resources.hpp"
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
    std::span<const uint8_t> palette, size_t position, bool typed, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed = nullptr)
{
    if (image.size() != 640u * 480 * 4 || palette.size() < 768 || (indexed && indexed->size() != 640u * 480))
    {
        return Result<bool>::failure(make_error("Invalid original sprite framebuffer/palette"));
    }
    if (texture_resources.active() && typed && palette.size() >= 828 && bytes.size() > 17 && bytes[16] == 'C' && std::equal(bytes.begin() + 8, bytes.begin() + 16, "SuperSp_"))
    {
        ResourceReader pal(palette), archive(bytes);
        const size_t address = pal.number(824, 4);
        if (address >= 58)
        {
            const size_t group = address - 58;
            const unsigned count = archive.number(group);
            for (unsigned id = 0; id < count; ++id)
            {
                if (group + archive.number(group + 2 + size_t(id) * 8, 4) != position) { continue; }
                auto output = image;
                auto result = texture_resources.draw_group(output, "portraits", id, left, top, palette, nullptr);
                if (!result) { return result; }
                image.swap(output);
                return result;
            }
        }
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
    std::vector<uint8_t> indices;
    if (indexed) { indices = *indexed; }
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
                if (indexed) { indices[dst / 4] = uint8_t(ink); }
                for (unsigned k = 0; k < 3; ++k)
                {
                    output[dst + k] = uint8_t(std::min(255u, unsigned(palette[ink * 3 + k]) * 255 / 63));
                }
                output[dst + 3] = 255;
            }
        }
    }
    image.swap(output);
    if (indexed) { indexed->swap(indices); }
    return Result<bool>::success(true);
}
inline Result<bool> draw_original_cel_legacy(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    std::span<const uint8_t> palette, unsigned id, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed = nullptr)
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
    return draw_original_sprite(image, cel.subspan(group + offset, length), palette, 0, false, left, top, zero, indexed);
}
inline Result<bool> draw_original_cel(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    std::span<const uint8_t> palette, unsigned id, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed = nullptr)
{
    if (!texture_resources.active()) { return draw_original_cel_legacy(image, cel, palette, id, left, top, zero, indexed); }
    if (image.size() != 640u * 480 * 4 || palette.size() < 768 || (indexed && indexed->size() != 640u * 480)) { return Result<bool>::failure(make_error("Invalid PNG UI framebuffer/palette")); }
    auto output = image;
    auto result = texture_resources.draw_ui(output, id, left, top, palette, indexed);
    if (!result) { return result; }
    image.swap(output);
    (void)cel;
    (void)zero;
    return result;
}
}    //namespace s2ne
