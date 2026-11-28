#include "original_sprite.h"

namespace s2ne
{
auto draw_original_sprite(std::vector<uint8_t>& image, std::span<const uint8_t> bytes,
    std::span<const uint8_t> palette, size_t position, bool typed, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed) -> Result<bool>
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
                if (skip || dx < 0 || dx >= stride || dy < 0 || dy >= framebuffer_height(image))
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

auto draw_original_cel_legacy(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    std::span<const uint8_t> palette, unsigned id, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed) -> Result<bool>
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

auto draw_original_panel_shadow(std::vector<uint8_t>& image, const std::vector<uint8_t>& palette,
    int width, int height, int x, int y, std::vector<uint8_t>* indexed) -> Result<bool>
{
    if (image.size() != 640u * 480 * 4 || palette.size() < 768 || (indexed && indexed->size() != 640u * 480))
    {
        return Result<bool>::failure(make_error("424D47 invalid framebuffer or palette"));
    }
    const int side_height = height - 10;
    if (side_height <= 0 || side_height > 468 || width <= 0 || width > 630) { return Result<bool>::success(false); }
    if (x < 8 && y + side_height + 18 >= 479) { return Result<bool>::success(false); }
    OriginalPalette shades(palette);
    for (int row = y + 8; row < y + side_height + 16; ++row)
    {
        const int left = x < 8 ? 0 : x - 8;
        int right = left + 8;
        if (row >= y + side_height + 8) { right = left + width; }
        else if (x < 8) { continue; }
        for (int column = left; column < right; ++column)
        {
            if (column < 0 || column >= 640 || row < 0 || row >= 480) { continue; }
            const size_t pixel = size_t(row) * 640 + column, p = pixel * 4;
            if (indexed)
            {
                const unsigned mapped = shades.map(6, indexed->at(pixel));
                indexed->at(pixel) = uint8_t(mapped);
                for (unsigned k = 0; k < 3; ++k) { image[p + k] = uint8_t(unsigned(palette[mapped * 3 + k]) * 255 / 63); }
                continue;
            }
            bool matched = false;
            for (unsigned ink = 0; ink < 256; ++ink)
            {
                if (image[p] != unsigned(palette[ink * 3]) * 255 / 63 || image[p + 1] != unsigned(palette[ink * 3 + 1]) * 255 / 63
                    || image[p + 2] != unsigned(palette[ink * 3 + 2]) * 255 / 63) { continue; }
                const unsigned mapped = shades.map(6, ink);
                for (unsigned k = 0; k < 3; ++k) { image[p + k] = uint8_t(unsigned(palette[mapped * 3 + k]) * 255 / 63); }
                matched = true;
                break;
            }
            if (!matched)
            {
                for (unsigned k = 0; k < 3; ++k) { image[p + k] = uint8_t((unsigned(image[p + k]) + unsigned(palette[32 * 3 + k]) * 255 / 63) / 2); }
            }
        }
    }
    return Result<bool>::success(true);
}

auto draw_original_cel_shadow(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    const std::vector<uint8_t>& palette, unsigned id, int x, int y, std::vector<uint8_t>* indexed) -> Result<bool>
{
    if (palette.size() < 820 || id < 142) { return Result<bool>::failure(make_error("424D47 invalid CEL directory")); }
    ResourceReader pal(palette), data(cel);
    const size_t group = pal.number(816, 4) - 57;
    if (!data.extent(group, 2) || id - 142 >= data.number(group)) { return Result<bool>::failure(make_error("424D47 invalid CEL ID")); }
    const size_t panel = group + data.number(group + 2 + size_t(id - 142) * 8, 4);
    if (!data.extent(panel, 4)) { return Result<bool>::failure(make_error("424D47 invalid CEL extent")); }
    return draw_original_panel_shadow(image, palette, int(data.number(panel)) + 1, int(data.number(panel + 2)) + 1, x, y, indexed);
}

auto draw_original_cel(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    std::span<const uint8_t> palette, unsigned id, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed) -> Result<bool>
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
