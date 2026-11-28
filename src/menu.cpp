#include "menu.h"

namespace s2ne
{
auto Menu::word(size_t p) const -> unsigned { return bin.at(p) | (unsigned(bin.at(p + 1)) << 8); }

auto Menu::dword(size_t p) const -> unsigned { return word(p) | (word(p + 2) << 16); }

auto Menu::entry(unsigned id) const -> size_t
{
    size_t p = 808 + 5 * 8 + (id - 1) * 8;
    unsigned value = pal.at(p) | (unsigned(pal.at(p + 1)) << 8) | (unsigned(pal.at(p + 2)) << 16) | (unsigned(pal.at(p + 3)) << 24);
    return value - 11 - (id - 1);
}

auto Menu::create(const std::filesystem::path& root) -> Result<Menu>
{
    Menu result;
    auto bin = read_shared_resource(root / "SUPERSP.BIN");
    if (!bin)
    {
        return Result<Menu>::failure(bin.error());
    }
    auto pal = read_shared_resource(root / "SUPERSP.PAL");
    if (!pal)
    {
        return Result<Menu>::failure(pal.error());
    }
    result.bin = std::move(bin).value();
    result.pal = std::move(pal).value();
    auto title = DiscResources(root).try_dat_entry(Disc::one, 19);
    if (!title) { return Result<Menu>::failure(title.error()); }
    const auto& pcx = title.value();
    if (pcx.size() < 769 || pcx[pcx.size() - 769] != 12)
    {
        return Result<Menu>::failure(make_error("Missing title PCX palette"));
    }
    for (size_t i = 0; i < result.title_palette_.size(); ++i)
    {
        result.title_palette_[i] = opening_palette_channel(uint8_t(pcx[pcx.size() - 768 + i] >> 2));
    }
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
        return Result<Menu>::failure(p.error());
    }
    result.count = b.number(result.strings);
    if (!b.extent(result.strings + 2, size_t(result.count) * 8) || !b.extent(result.index, 2) || !b.extent(result.glyphs, 1))
    {
        return Result<Menu>::failure(b.error());
    }
    for (unsigned id : ids)
    {
        if (id >= result.count)
        {
            return Result<Menu>::failure(make_error("Missing original menu string"));
        }
    }
    for (unsigned id = 0; id < result.count; ++id)
    {
        auto text = result.try_text_bytes(id);
        if (!text)
        {
            return Result<Menu>::failure(text.error());
        }
        result.texts_.push_back(std::move(text).value());
    }
    return Result<Menu>::success(std::move(result));
}

Menu::Menu(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto Menu::move(int delta) -> void
{
    selected = std::clamp(selected + delta, 0, 4);
    activated = -1;
}

auto Menu::hit(float x, float y) const -> int { return x >= 266 && x <= 386 && y >= 283 && y < 433 ? int(y - 283) / 30 : -1; }

auto Menu::mouse(float x, float y, bool click) -> bool
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

auto Menu::activate() -> unsigned
{
    activated = selected;
    return targets.at(selected);
}

auto Menu::try_text_bytes(unsigned id) const -> Result<std::vector<uint8_t>>
{
    if (id >= count)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid original string ID"));
    }
    if (id < texts_.size()) { return Result<std::vector<uint8_t>>::success(texts_[id]); }
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

auto Menu::text_bytes(unsigned id) const -> const std::vector<uint8_t>&
{
    if (id >= texts_.size()) { fatal("Invalid original string ID"); }
    return texts_[id];
}

auto Menu::draw_text(std::vector<uint8_t>& image, unsigned id, int x, int y, unsigned color) const -> void
{
    draw_big5(image, text_bytes(id), x, y, color);
}

auto Menu::set_simplified_chinese(bool enabled, const std::filesystem::path& dictionary_directory) -> void
{
    simplified_chinese = enabled;
    SimpleCC::configure_once(dictionary_directory, enabled);
}

auto Menu::try_draw_small_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding, std::vector<uint8_t>* indexed) const -> s2ne::Result<bool>
{
    const int stride = framebuffer_width(image);
    if (!stride || color > 255 || (indexed && indexed->size() != image.size() / 4))
    {
        return s2ne::Result<bool>::failure(make_error("Invalid text framebuffer or palette color"));
    }
    if (texture_resources.small_font_ready() && !indexed)
    {
        const auto utf8 = SimpleCC::big5_to_utf8(text);
        if (!utf8.empty())
        {
            const auto translated = SimpleCC::enabled() ? SimpleCC::shared().convert(utf8) : utf8;
            const SDL_Color foreground{ uint8_t(std::min(255u, unsigned(pal.at(color * 3)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(color * 3 + 1)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(color * 3 + 2)) * 255 / 63)), 255 };
            auto rendered = texture_resources.draw_small_text(image, translated, x + (original_padding ? 1 : 0), y + (original_padding ? 1 : 0), foreground);
            return rendered;
        }
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
        if (texture_resources.active())
        {
            const auto glyph = std::span<const uint8_t>(bin).subspan(bits, size_t(width / 8) * height);
            auto drawn = texture_resources.draw_glyph(image, glyph, width, height, x + (original_padding ? 1 : 0), y + (original_padding && height == 15 ? 1 : 0), pal, color, indexed);
            if (!drawn) { return drawn; }
            x += int(width);
            continue;
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
                if (dx < 0 || dx >= stride || dy < 0 || dy >= 480)
                {
                    continue;
                }
                size_t dst = (size_t(dy) * stride + dx) * 4;
                if (indexed) { indexed->at(dst / 4) = uint8_t(color); }
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

auto Menu::try_draw_small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding, std::vector<uint8_t>* indexed) const -> s2ne::Result<bool>
{
    auto output = image;
    std::vector<uint8_t> indices;
    if (indexed) { indices = *indexed; }
    auto result = try_draw_small_validated(output, text, x, y, color, original_padding, indexed ? &indices : nullptr);
    if (result)
    {
        image.swap(output);
        if (indexed) { indexed->swap(indices); }
    }
    return result;
}

auto Menu::draw_small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding) const -> void
{
    auto result = try_draw_small(image, text, x, y, color, original_padding);
    if (!result)
    {
        report_error(result.error());
    }
}

auto Menu::try_draw_small_fixed_spaces(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding) const -> s2ne::Result<bool>
{
    for (size_t p = 0; p < text.size();)
    {
        if (text[p] == ' ')
        {
            x += 8;
            ++p;
            continue;
        }
        unsigned width = text[p] > 0x80 ? 2 : 1;
        if (p + width > text.size()) { return s2ne::Result<bool>::failure(make_error("Truncated Big5 fixed text")); }
        std::vector<uint8_t> glyph(text.begin() + p, text.begin() + p + width);
        auto drawn = try_draw_small(image, glyph, x, y, color, original_padding);
        if (!drawn) { return drawn; }
        x += width == 2 ? 16 : 8;
        p += width;
    }
    return s2ne::Result<bool>::success(true);
}

auto Menu::draw_small_fixed_spaces(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding) const -> void
{
    auto drawn = try_draw_small_fixed_spaces(image, text, x, y, color, original_padding);
    if (!drawn) { report_error(drawn.error()); }
}

auto Menu::try_draw_big5_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
    int x, int y, unsigned color, std::vector<uint8_t>* indexed) const -> s2ne::Result<bool>
{
    const int stride = framebuffer_width(image);
    if (!stride || color > 255 || (indexed && indexed->size() != image.size() / 4))
    {
        return s2ne::Result<bool>::failure(make_error("Invalid text framebuffer or palette color"));
    }
    if (texture_resources.font_ready() && !indexed)
    {
        const auto utf8 = SimpleCC::big5_to_utf8(text);
        if (!utf8.empty())
        {
            const auto translated = SimpleCC::enabled() ? SimpleCC::shared().convert(utf8) : utf8;
            const SDL_Color foreground{ uint8_t(std::min(255u, unsigned(pal.at(color * 3)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(color * 3 + 1)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(color * 3 + 2)) * 255 / 63)), 255 };
            auto rendered = texture_resources.draw_large_text(image, translated, x, y, foreground);
            return rendered;
        }
    }
    for (size_t p = 0; p < text.size();)
    {
        unsigned lead = text.at(p++), code = 0;
        if (lead <= 0x80)
        {
            size_t bits = entry(16) + std::max(30u, lead) * 48;
            if (texture_resources.active())
            {
                auto drawn = texture_resources.draw_glyph(image, std::span<const uint8_t>(bin).subspan(bits, 48), 16, 24, x, y, pal, color, indexed);
                if (!drawn) { return drawn; }
                x += 16;
                continue;
            }
            for (int gy = 0; gy < 24; ++gy)
            {
                for (int gx = 0; gx < 16; ++gx)
                {
                    if (!(bin.at(bits + gy * 2 + gx / 8) & (128 >> (gx % 8))))
                    {
                        continue;
                    }
                    if (x + gx < 0 || x + gx >= stride || y + gy < 0 || y + gy >= 480)
                    {
                        continue;
                    }
                    size_t dst = (size_t(y + gy) * stride + x + gx) * 4;
                    if (indexed) { indexed->at(dst / 4) = uint8_t(color); }
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
        if (texture_resources.active())
        {
            auto drawn = texture_resources.draw_glyph(image, std::span<const uint8_t>(bin).subspan(bits, 72), 24, 24, x, y, pal, color, indexed);
            if (!drawn) { return drawn; }
            x += 24;
            continue;
        }
        for (int gy = 0; gy < 24; ++gy)
        {
            for (int gx = 0; gx < 24; ++gx)
            {
                if (!(bin.at(bits + gy * 3 + gx / 8) & (128 >> (gx % 8))))
                {
                    continue;
                }
                if (x + gx < 0 || x + gx >= stride || y + gy < 0 || y + gy >= 480)
                {
                    continue;
                }
                size_t dst = (size_t(y + gy) * stride + x + gx) * 4;
                if (indexed) { indexed->at(dst / 4) = uint8_t(color); }
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

auto Menu::try_draw_big5(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
    int x, int y, unsigned color, std::vector<uint8_t>* indexed) const -> s2ne::Result<bool>
{
    auto output = image;
    std::vector<uint8_t> indices;
    if (indexed) { indices = *indexed; }
    auto result = try_draw_big5_validated(output, text, x, y, color, indexed ? &indices : nullptr);
    if (result)
    {
        image.swap(output);
        if (indexed) { indexed->swap(indices); }
    }
    return result;
}

auto Menu::draw_big5(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
    int x, int y, unsigned color) const -> void
{
    auto result = try_draw_big5(image, text, x, y, color);
    if (!result)
    {
        report_error(result.error());
    }
}

auto Menu::try_draw_original_large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, std::optional<unsigned> shadow, std::vector<uint8_t>* indexed) const -> s2ne::Result<bool>
{
    if (!framebuffer_width(image) || color > 255 || (indexed && indexed->size() != image.size() / 4))
    {
        return s2ne::Result<bool>::failure(make_error("Invalid text framebuffer or palette color"));
    }
    if (texture_resources.font_ready() && !indexed)
    {
        const auto utf8 = SimpleCC::big5_to_utf8(text);
        if (!utf8.empty())
        {
            const auto translated = SimpleCC::enabled() ? SimpleCC::shared().convert(utf8) : utf8;
            const SDL_Color foreground{ uint8_t(std::min(255u, unsigned(pal.at(color * 3)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(color * 3 + 1)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(color * 3 + 2)) * 255 / 63)), 255 };
            if (shadow)
            {
                if (*shadow > 255) { return s2ne::Result<bool>::failure(make_error("Invalid text shadow color")); }
                const SDL_Color shadow_color{ uint8_t(std::min(255u, unsigned(pal.at(*shadow * 3)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(*shadow * 3 + 1)) * 255 / 63)), uint8_t(std::min(255u, unsigned(pal.at(*shadow * 3 + 2)) * 255 / 63)), 255 };
                return texture_resources.draw_text_batch(image, [&](std::vector<uint8_t>& output)
                    {
                        auto rendered_shadow = texture_resources.draw_large_text(output, translated, x + 1, y + 1, shadow_color);
                        if (!rendered_shadow) { return rendered_shadow; }
                        return texture_resources.draw_large_text(output, translated, x, y, foreground);
                    });
            }
            else
            {
                auto rendered = texture_resources.draw_large_text(image, translated, x, y, foreground);
                return rendered;
            }
        }
    }
    auto output = image;
    std::vector<uint8_t> indices;
    if (indexed) { indices = *indexed; }
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
                    auto rendered = try_draw_small_validated(output, glyph, gx + 1, gy + 1, *shadow, true, indexed ? &indices : nullptr);
                    if (!rendered)
                    {
                        return rendered;
                    }
                }
            }
            {
                auto rendered = try_draw_small_validated(output, glyph, gx, gy, color, true, indexed ? &indices : nullptr);
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
                    auto rendered = try_draw_big5_validated(output, glyph, ink_x + 1, ink_y + 1, *shadow, indexed ? &indices : nullptr);
                    if (!rendered)
                    {
                        return rendered;
                    }
                }
            }
            {
                auto rendered = try_draw_big5_validated(output, glyph, ink_x, ink_y, color, indexed ? &indices : nullptr);
                if (!rendered)
                {
                    return rendered;
                }
            }
        }
        x += lead > 128 ? 24 : 16;
    }
    image.swap(output);
    if (indexed) { indexed->swap(indices); }
    return s2ne::Result<bool>::success(true);
}

auto Menu::draw_original_large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, std::optional<unsigned> shadow) const -> void
{
    auto result = try_draw_original_large(image, text, x, y, color, shadow);
    if (!result)
    {
        report_error(result.error());
    }
}

auto Menu::draw_original_wrapped_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, unsigned rows, unsigned columns, int row_height, std::optional<unsigned> shadow) const -> s2ne::Result<bool>
{
    if (texture_resources.active() && texture_resources.font_ready())
    {
        size_t p = 0;
        for (unsigned row = 0; row < rows; ++row)
        {
            int compensation = 0;
            for (unsigned cell = 0, units = 1; units < columns * 2; ++cell)
            {
                if (p >= text.size() || !text[p] || text[p] == '\n') { return s2ne::Result<bool>::success(true); }
                const auto lead = text[p++];
                if (lead == '*')
                {
                    if (cell == 0) { --cell; }
                    else
                    {
                        break;
                    }
                    continue;
                }
                std::vector<uint8_t> glyph{ uint8_t(std::max(30u, unsigned(lead))) };
                if (lead > 128)
                {
                    if (p >= text.size())
                    {
                        return s2ne::Result<bool>::failure(make_error("Truncated Big5 wrapped text"));
                    }
                    glyph.push_back(text[p++]);
                    units += 2;
                }
                else
                {
                    ++units;
                }
                auto drawn = try_draw_original_large(image, glyph, x + int(cell) * 28 - compensation, y + int(row) * row_height, color, shadow);
                if (!drawn) { return drawn; }
                if (lead <= 128) { compensation += 8; }
            }
        }
        return s2ne::Result<bool>::success(true);
    }
    // 423AC5 uses 28-pixel cells, compensating eight pixels after each ASCII.
    size_t p = 0;
    for (unsigned row = 0; row < rows; ++row)
    {
        int compensation = 0;
        for (int cell = 0, units = 1; units < int(columns * 2); ++cell)
        {
            if (p >= text.size() || !text[p] || text[p] == '\n')
            {
                return s2ne::Result<bool>::success(true);
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
                if (p >= text.size()) { return s2ne::Result<bool>::failure(make_error("Truncated Big5 wrapped text")); }
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
                    auto drawn = try_draw_small(image, glyph, gx + 1, gy + 5, *shadow, true);
                    if (!drawn) { return drawn; }
                }
                auto drawn = try_draw_small(image, glyph, gx, gy + 4, color, true);
                if (!drawn) { return drawn; }
            }
            else
            {
                gx += 1;
                gy += lead <= 128 ? 1 : 0;
                if (shadow)
                {
                    auto drawn = try_draw_big5(image, glyph, gx + 1, gy + 1, *shadow);
                    if (!drawn) { return drawn; }
                }
                auto drawn = try_draw_big5(image, glyph, gx, gy, color);
                if (!drawn) { return drawn; }
            }
            if (lead <= 128)
            {
                compensation += 8;
            }
        }
    }
    return s2ne::Result<bool>::success(true);
}

auto Menu::try_draw_original_wrapped(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, unsigned rows, unsigned columns, int row_height, std::optional<unsigned> shadow) const -> s2ne::Result<bool>
{
    if (texture_resources.active() && texture_resources.font_ready())
    {
        return texture_resources.draw_text_batch(image, [&](std::vector<uint8_t>& output)
            {
                return draw_original_wrapped_validated(output, text, x, y, color, rows, columns, row_height, shadow);
            });
    }
    return draw_original_wrapped_validated(image, text, x, y, color, rows, columns, row_height, shadow);
}

auto Menu::draw_original_wrapped(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, unsigned rows, unsigned columns, int row_height, std::optional<unsigned> shadow) const -> void
{
    auto drawn = try_draw_original_wrapped(image, text, x, y, color, rows, columns, row_height, shadow);
    if (!drawn) { report_error(drawn.error()); }
}

auto Menu::draw_banner_ascii(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y) const -> void
{
    // 4239AD -> 4235AE: ASCII x+2, padded glyph (1,1), shadow offset (1,1).
    for (uint8_t letter : text)
    {
        draw_big5(image, { letter }, x + 4, y + 2, 36);
        draw_big5(image, { letter }, x + 3, y + 1, 33);
        x += 16;
    }
}

auto Menu::draw(std::vector<uint8_t>& image) const -> void
{
    // 40DFD6/40E057: both normal and selected rows use style1, edge3.
    for (unsigned row = 0; row < 5; ++row)
    {
        const auto text = SimpleCC::big5_to_utf8(text_bytes(ids[row]));
        const unsigned palette_index = row == static_cast<unsigned>(selected) ? 2 : 15;
        const SDL_Color color{ title_palette_[palette_index * 3], title_palette_[palette_index * 3 + 1], title_palette_[palette_index * 3 + 2], 255 };
        auto drawn = texture_resources.draw_large_text(image, SimpleCC::enabled() ? SimpleCC::shared().convert(text) : text, 278, 286 + int(row) * 30, color);
        if (!drawn) { fatal(drawn.error()); }
    }
}
}    //namespace s2ne
