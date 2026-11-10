#pragma once
#include "resource_result.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>
namespace s2ne
{
class OriginalMenu
{
    std::vector<uint8_t> bin, pal;
    size_t strings{ }, index{ }, glyphs{ };
    unsigned count{ };
    unsigned word(size_t p) const { return bin.at(p) | (unsigned(bin.at(p + 1)) << 8); }
    unsigned dword(size_t p) const { return word(p) | (word(p + 2) << 16); }
    size_t entry(unsigned id) const
    {
        size_t p = 808 + 5 * 8 + (id - 1) * 8;
        unsigned value = pal.at(p) | (unsigned(pal.at(p + 1)) << 8) | (unsigned(pal.at(p + 2)) << 16) | (unsigned(pal.at(p + 3)) << 24);
        return value - 11 - (id - 1);
    }

public:
    bool small_font = false;
    int selected = 0;
    int activated = -1;
    static constexpr std::array<unsigned, 5> ids{ 212, 213, 214, 123, 131 };
    static constexpr std::array<unsigned, 5> targets{ 0x40E53C, 0x4371E6, 0x40EBF3, 0x434103, 0x43183B };
    OriginalMenu() = default;
    static Result<OriginalMenu> create(const std::filesystem::path& root)
    {
        OriginalMenu result;
        auto bin = read_resource(root / "SUPERSP.BIN");
        if (!bin)
        {
            return Result<OriginalMenu>::failure(bin.error());
        }
        auto pal = read_resource(root / "SUPERSP.PAL");
        if (!pal)
        {
            return Result<OriginalMenu>::failure(pal.error());
        }
        result.bin = std::move(bin).value();
        result.pal = std::move(pal).value();
        ResourceReader p(result.pal), b(result.bin);
        auto entry = [&](unsigned id) -> size_t
        {
            unsigned raw = p.number(848 + (id - 1) * 8, 4), base = 11 + id - 1;
            if (raw < base)
            {
                p.reject("Invalid menu resource address");
                return 0;
            }
            return raw - base;
        };
        result.strings = entry(5);
        result.index = entry(11);
        result.glyphs = entry(13);
        if (!p)
        {
            return Result<OriginalMenu>::failure(p.error());
        }
        result.count = b.number(result.strings);
        if (!b.extent(result.strings + 2, size_t(result.count) * 8) || !b.extent(result.index, 2) || !b.extent(result.glyphs, 1))
        {
            return Result<OriginalMenu>::failure(b.error());
        }
        for (unsigned id : ids)
        {
            if (id >= result.count)
            {
                return Result<OriginalMenu>::failure(make_error("Missing original menu string"));
            }
        }
        for (unsigned id = 0; id < result.count; ++id)
        {
            auto text = result.try_text_bytes(id);
            if (!text)
            {
                return Result<OriginalMenu>::failure(text.error());
            }
        }
        return Result<OriginalMenu>::success(std::move(result));
    }
    explicit OriginalMenu(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    void move(int delta)
    {
        selected = std::clamp(selected + delta, 0, 4);
        activated = -1;
    }
    int hit(float x, float y) const { return x >= 266 && x <= 386 && y >= 283 && y < 433 ? int(y - 283) / 30 : -1; }
    bool mouse(float x, float y, bool click)
    {
        int row = hit(x, y);
        if (row < 0)
        {
            return false;
        }
        bool same = row == selected;
        selected = row;
        activated = -1;
        return click && same;
    }
    unsigned activate()
    {
        activated = selected;
        return targets.at(selected);
    }
    Result<std::vector<uint8_t>> try_text_bytes(unsigned id) const
    {
        if (id >= count)
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid original string ID"));
        }
        ResourceReader reader(bin);
        size_t offset = reader.number(strings + 2 + id * 8, 4);
        if (!reader)
        {
            return Result<std::vector<uint8_t>>::failure(reader.error());
        }
        size_t base = strings + 2 + size_t(count) * 8;
        if (base > bin.size() || offset > bin.size() - base)
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid menu text extent"));
        }
        size_t text = base + offset, end = text;
        while (end < bin.size() && bin[end])
        {
            ++end;
        }
        if (end == bin.size())
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Unterminated menu text"));
        }
        return Result<std::vector<uint8_t>>::success({ bin.begin() + text, bin.begin() + end });
    }
    std::vector<uint8_t> text_bytes(unsigned id) const
    {
        auto result = try_text_bytes(id);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    void draw_text(std::vector<uint8_t>& image, unsigned id, int x, int y, unsigned color) const
    {
        draw_big5(image, text_bytes(id), x, y, color);
    }
    // sub_422402: BIN 12 contains 16x16 Big5 glyphs; BIN 15 contains 8x15 ASCII.
    s2ne::Result<bool> try_draw_small_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false) const
    {
        if (image.size() != 640u * 480 * 4 || color > 255)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid text framebuffer or palette color"));
        }
        for (size_t p = 0; p < text.size();)
        {
            unsigned lead = text.at(p++);
            if (!lead || lead == '\n')
            {
                break;
            }
            unsigned width = 8, height = 15;
            size_t bits = entry(15) + std::max(30u, lead) * 15;
            if (lead > 128)
            {
                if (p >= text.size())
                {
                    return s2ne::Result<bool>::failure(make_error("Truncated Big5 text"));
                }
                unsigned trail = text.at(p++), base = 0, first = 0;
                if (lead == 0xC6 && trail >= 0xA1)
                {
                    base = 0x8001;
                    first = lead;
                    trail -= 0xA1;
                }
                else
                {
                    if (lead >= 0xA1 && lead <= 0xA3)
                    {
                        base = 0x8400;
                        first = 0xA1;
                    }
                    else if (lead >= 0xA4 && lead <= 0xC6)
                    {
                        base = 0x8800;
                        first = 0xA4;
                    }
                    else if (lead >= 0xC7 && lead <= 0xC8)
                    {
                        base = 0x805F;
                        first = 0xC7;
                    }
                    else if (lead >= 0xC9 && lead <= 0xF9)
                    {
                        base = 0x9D19;
                        first = 0xC9;
                    }
                    else
                    {
                        return s2ne::Result<bool>::failure(make_error("Invalid small-font Big5 lead"));
                    }
                    if (!((trail >= 0x40 && trail <= 0x7E) || (trail >= 0xA1 && trail <= 0xFE)))
                    {
                        return s2ne::Result<bool>::failure(make_error("Invalid small-font Big5 trail"));
                    }
                    trail -= trail <= 0x7E ? 0x40 : 0x62;
                }
                unsigned glyph = word(index + (base + (lead - first) * 157 + trail - 0x8000) * 2);
                if (!glyph)
                {
                    return s2ne::Result<bool>::failure(make_error("Missing small-font glyph"));
                }
                bits = entry(12) + (glyph - 1) * 32;
                width = height = 16;
            }
            for (unsigned gy = 0; gy < height; ++gy)
            {
                for (unsigned gx = 0; gx < width; ++gx)
                {
                    if (!(bin.at(bits + gy * (width / 8) + gx / 8) & (128 >> (gx % 8))))
                    {
                        continue;
                    }
                    int dx = x + int(gx) + (original_padding ? 1 : 0), dy = y + int(gy) + (original_padding && height == 15 ? 1 : 0);
                    if (dx < 0 || dx >= 640 || dy < 0 || dy >= 480)
                    {
                        continue;
                    }
                    size_t dst = (size_t(dy) * 640 + dx) * 4;
                    for (unsigned c = 0; c < 3; ++c)
                    {
                        image.at(dst + c) = uint8_t(std::min(255u, unsigned(pal.at(color * 3 + c)) * 255 / 63));
                    }
                    image.at(dst + 3) = 255;
                }
            }
            x += int(width);
        }

        return s2ne::Result<bool>::success(true);
    }
    s2ne::Result<bool> try_draw_small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false) const
    {
        auto output = image;
        auto result = try_draw_small_validated(output, text, x, y, color, original_padding);
        if (result)
        {
            image.swap(output);
        }
        return result;
    }
    void draw_small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false) const
    {
        auto result = try_draw_small(image, text, x, y, color, original_padding);
        if (!result)
        {
            report_error(result.error());
        }
    }
    s2ne::Result<bool> try_draw_big5_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
        int x, int y, unsigned color) const
    {
        if (image.size() != 640u * 480 * 4 || color > 255)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid text framebuffer or palette color"));
        }
        for (size_t p = 0; p < text.size();)
        {
            unsigned lead = text.at(p++), code = 0;
            if (lead <= 0x80)
            {
                size_t bits = entry(16) + std::max(30u, lead) * 48;
                for (int gy = 0; gy < 24; ++gy)
                {
                    for (int gx = 0; gx < 16; ++gx)
                    {
                        if (!(bin.at(bits + gy * 2 + gx / 8) & (128 >> (gx % 8))))
                        {
                            continue;
                        }
                        if (x + gx < 0 || x + gx >= 640 || y + gy < 0 || y + gy >= 480)
                        {
                            continue;
                        }
                        size_t dst = (size_t(y + gy) * 640 + x + gx) * 4;
                        for (unsigned c = 0; c < 3; ++c)
                        {
                            image.at(dst + c) = uint8_t(unsigned(pal.at(color * 3 + c)) * 255 / 63);
                        }
                        image.at(dst + 3) = 255;
                    }
                }
                x += 16;
                continue;
            }
            if (p >= text.size())
            {
                return s2ne::Result<bool>::failure(make_error("Truncated Big5 text"));
            }
            unsigned trail = text.at(p++);
            if (lead == 0xC6 && trail >= 0xA1)
            {
                code = 0x8001 + trail - 0xA1;
            }
            else
            {
                unsigned base = 0, first = 0;
                if (lead >= 0xA1 && lead <= 0xA3)
                {
                    base = 0x8400;
                    first = 0xA1;
                }
                else if (lead >= 0xA4 && lead <= 0xC6)
                {
                    base = 0x8800;
                    first = 0xA4;
                }
                else if (lead >= 0xC7 && lead <= 0xC8)
                {
                    base = 0x805F;
                    first = 0xC7;
                }
                else if (lead >= 0xC9 && lead <= 0xF9)
                {
                    base = 0x9D19;
                    first = 0xC9;
                }
                else
                {
                    return s2ne::Result<bool>::failure(make_error("Invalid original Big5 lead"));
                }
                if (!((trail >= 0x40 && trail <= 0x7E) || (trail >= 0xA1 && trail <= 0xFE)))
                {
                    return s2ne::Result<bool>::failure(make_error("Invalid original Big5 trail"));
                }
                code = base + (lead - first) * 157 + trail - (trail <= 0x7E ? 0x40 : 0x62);
            }
            unsigned glyph = word(index + (code - 0x8000) * 2);
            if (!glyph)
            {
                return s2ne::Result<bool>::failure(make_error("Missing original dialogue glyph"));
            }
            size_t bits = glyphs + (glyph - 1) * 72;
            for (int gy = 0; gy < 24; ++gy)
            {
                for (int gx = 0; gx < 24; ++gx)
                {
                    if (!(bin.at(bits + gy * 3 + gx / 8) & (128 >> (gx % 8))))
                    {
                        continue;
                    }
                    if (x + gx < 0 || x + gx >= 640 || y + gy < 0 || y + gy >= 480)
                    {
                        continue;
                    }
                    size_t dst = (size_t(y + gy) * 640 + x + gx) * 4;
                    for (unsigned c = 0; c < 3; ++c)
                    {
                        image.at(dst + c) = uint8_t(std::min(255u, unsigned(pal.at(color * 3 + c)) * 255 / 63));
                    }
                    image.at(dst + 3) = 255;
                }
            }
            x += 24;
        }

        return s2ne::Result<bool>::success(true);
    }
    s2ne::Result<bool> try_draw_big5(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
        int x, int y, unsigned color) const
    {
        auto output = image;
        auto result = try_draw_big5_validated(output, text, x, y, color);
        if (result)
        {
            image.swap(output);
        }
        return result;
    }
    void draw_big5(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
        int x, int y, unsigned color) const
    {
        auto result = try_draw_big5(image, text, x, y, color);
        if (!result)
        {
            report_error(result.error());
        }
    }
    s2ne::Result<bool> try_draw_original_large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, std::optional<unsigned> shadow = std::nullopt) const
    {
        if (image.size() != 640u * 480 * 4 || color > 255)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid text framebuffer or palette color"));
        }
        auto output = image;
        for (size_t p = 0; p < text.size();)
        {
            unsigned lead = text.at(p++);
            if (!lead || lead == '\n')
            {
                break;
            }
            std::vector<uint8_t> glyph{ uint8_t(lead) };
            if (lead > 128)
            {
                if (p >= text.size())
                {
                    return s2ne::Result<bool>::failure(make_error("Truncated Big5 text"));
                }
                glyph.push_back(text.at(p++));
            }
            int ink_x = x + (lead > 128 ? 1 : 3), ink_y = y + (lead > 128 ? 0 : 1);
            if (small_font)
            {
                int gx = x + (lead > 128 ? 4 : 2), gy = y + 4;
                if (shadow)
                {
                    {
                        auto rendered = try_draw_small(output, glyph, gx + 1, gy + 1, *shadow, true);
                        if (!rendered)
                        {
                            return rendered;
                        }
                    }
                }
                {
                    auto rendered = try_draw_small(output, glyph, gx, gy, color, true);
                    if (!rendered)
                    {
                        return rendered;
                    }
                }
            }
            else
            {
                if (shadow)
                {
                    {
                        auto rendered = try_draw_big5(output, glyph, ink_x + 1, ink_y + 1, *shadow);
                        if (!rendered)
                        {
                            return rendered;
                        }
                    }
                }
                {
                    auto rendered = try_draw_big5(output, glyph, ink_x, ink_y, color);
                    if (!rendered)
                    {
                        return rendered;
                    }
                }
            }
            x += lead > 128 ? 24 : 16;
        }
        image.swap(output);
        return s2ne::Result<bool>::success(true);
    }
    void draw_original_large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, std::optional<unsigned> shadow = std::nullopt) const
    {
        auto result = try_draw_original_large(image, text, x, y, color, shadow);
        if (!result)
        {
            report_error(result.error());
        }
    }
    void draw_original_wrapped(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, unsigned rows, unsigned columns, int row_height, std::optional<unsigned> shadow = std::nullopt) const
    {
        // 423AC5 uses 28-pixel cells, compensating eight pixels after each ASCII.
        size_t p = 0;
        for (unsigned row = 0; row < rows; ++row)
        {
            int compensation = 0;
            for (int cell = 0, units = 1; units < int(columns * 2); ++cell)
            {
                if (p >= text.size() || !text[p] || text[p] == '\n')
                {
                    return;
                }
                unsigned lead = text[p++];
                if (lead == '*')
                {
                    if (cell == 0)
                    {
                        --cell;
                    }
                    else
                    {
                        break;
                    }
                    continue;
                }
                std::vector<uint8_t> glyph{ uint8_t(std::max(lead, 30u)) };
                if (lead > 128)
                {
                    glyph.push_back(text.at(p++));
                    units += 2;
                }
                else
                {
                    ++units;
                }
                int gx = x + cell * (small_font ? 20 : 28) - compensation, gy = y + int(row) * row_height;
                if (small_font)
                {
                    if (shadow)
                    {
                        draw_small(image, glyph, gx + 1, gy + 5, *shadow, true);
                    }
                    draw_small(image, glyph, gx, gy + 4, color, true);
                }
                else
                {
                    gx += 1;
                    gy += lead <= 128 ? 1 : 0;
                    if (shadow)
                    {
                        draw_big5(image, glyph, gx + 1, gy + 1, *shadow);
                    }
                    draw_big5(image, glyph, gx, gy, color);
                }
                if (lead <= 128)
                {
                    compensation += 8;
                }
            }
        }
    }
    void draw_banner_ascii(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y) const
    {
        // 4239AD -> 4235AE: ASCII x+2, padded glyph (1,1), shadow offset (1,1).
        for (uint8_t letter : text)
        {
            draw_big5(image, { letter }, x + 4, y + 2, 36);
            draw_big5(image, { letter }, x + 3, y + 1, 33);
            x += 16;
        }
    }
    void draw(std::vector<uint8_t>& image) const
    {
        // 40DFD6/40E057: both normal and selected rows use style1, edge3.
        for (unsigned row = 0; row < 5; ++row)
        {
            draw_original_large(image, text_bytes(ids[row]), 278, 286 + int(row) * 30, int(row) == selected ? 2 : 15, 3);
        }
    }
};
}    //namespace s2ne
