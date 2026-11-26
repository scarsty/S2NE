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
    const int stride = framebuffer_width(image);
    if (!stride || palette.size() < 768 || (indexed && indexed->size() != image.size() / 4))
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
    if (!reader || type > 1)
    {
        return Result<bool>::failure(make_error("Invalid original sprite header"));
    }
    const size_t minimum_row_bytes = type ? width : (size_t(width) + 126) / 127;
    if (position > bytes.size() || size_t(height) > (bytes.size() - position) / minimum_row_bytes)
    {
        return Result<bool>::failure(make_error("Truncated original sprite rows"));
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
                // 原版零游程始终表示底图透明，不能覆盖页面下方的已有像素。
                bool skip = !ink && (type || (zero == OriginalZeroRun::transparent && !(op & 128)));
                if (skip || dx < 0 || dx >= stride || dy < 0 || dy >= 480)
                {
                    continue;
                }
                size_t dst = (size_t(dy) * stride + dx) * 4;
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
    auto available = texture_resources.has_ui(id);
    if (!available) { return Result<bool>::failure(available.error()); }
    if (!available.value()) { return draw_original_cel_legacy(image, cel, palette, id, left, top, zero, indexed); }
    auto output = image;
    std::vector<uint8_t> indices;
    if (indexed) { indices = *indexed; }
    auto result = texture_resources.draw_ui(output, id, left, top, palette, indexed ? &indices : nullptr);
    if (!result) { return result; }
    image.swap(output);
    if (indexed) { indexed->swap(indices); }
    return result;
}
}    //namespace s2ne
