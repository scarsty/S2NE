#pragma once
#include "original_menu.hpp"
#include "original_palette.hpp"
#include <optional>
namespace s2ne
{
class OriginalConfirmation
{
    std::vector<uint8_t> cel_, pal_, text_;
    int x_, y_, selection_;
    std::optional<bool> result_;
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    s2ne::Result<bool> try_sprite_validated(std::vector<uint8_t>& image, unsigned id, int left, int top) const
    {
        size_t group = dword(pal_, 816) - 57, p = group + dword(cel_, group + 2 + (id - 142) * 8);
        unsigned width = word(cel_, p) + 1, height = word(cel_, p + 2) + 1;
        p += 4;
        for (unsigned y = 0; y < height; ++y)
        {
            unsigned x = 0;
            while (x < width)
            {
                unsigned op = cel_.at(p++), n = op & 127, value = 0;
                if (!op) { n = 1; }
                else if (!(op & 128))
                {
                    value = cel_.at(p++);
                }
                if (!n || n > width - x)
                {
                    return s2ne::Result<bool>::failure(make_error("Invalid confirmation sprite run"));
                }
                while (n--)
                {
                    unsigned c = op & 128 ? cel_.at(p++) : value;
                    int dx = left + int(x++), dy = top + int(y);
                    if (dx < 0 || dx >= 640 || dy < 0 || dy >= 480)
                    {
                        continue;
                    }
                    size_t dst = (size_t(dy) * 640 + dx) * 4;
                    for (unsigned k = 0; k < 3; ++k)
                    {
                        image.at(dst + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
                    }
                    image.at(dst + 3) = 255;
                }
            }
        }

        return s2ne::Result<bool>::success(true);
    }
    s2ne::Result<bool> try_sprite(std::vector<uint8_t>& image, unsigned id, int left, int top) const
    {
        auto output = image;
        auto result = try_sprite_validated(output, id, left, top);
        if (result)
        {
            image.swap(output);
        }
        return result;
    }
    void sprite(std::vector<uint8_t>& image, unsigned id, int left, int top) const
    {
        auto result = try_sprite(image, id, left, top);
        if (!result)
        {
            report_error(result.error());
        }
    }

public:
    OriginalConfirmation(const std::filesystem::path& root, const OriginalMenu& font, unsigned string_id, int x, int y, bool default_yes = true) : text_(font.text_bytes(string_id)), x_(x), y_(y), selection_(default_yes ? 0 : 1)
    {
        auto read = [](const auto& path)
        {
            return read_resource(path).value();
        };
        cel_ = read(root / "SUPERSP.CEL");
        pal_ = read(root / "SUPERSP.PAL");
        pal_.at(0) = pal_.at(1) = pal_.at(2) = 0;
    }
    void toggle() { selection_ ^= 1; }
    void confirm() { result_ = selection_ == 0; }
    void cancel() { result_ = false; }
    std::optional<bool> result() const { return result_; }
    int selection() const { return selection_; }
    void mouse(float x, float y, bool left_click)
    {
        if (y < y_ + 4 || y > y_ + 32)
        {
            return;
        }
        int row = x >= x_ + 169 && x <= x_ + 196 ? 0 : x >= x_ + 200 && x <= x_ + 227 ? 1 :
                                                                                        -1;
        if (row < 0)
        {
            return;
        }
        selection_ = row;
        if (left_click)
        {
            confirm();
        }
    }
    s2ne::Result<bool> try_draw(std::vector<uint8_t>& image, const OriginalMenu& font) const
    {
        // 431997 calls 475970 with fill=0: outline the embedded O/X artwork.
        size_t group = dword(pal_, 816) - 57, p = group + dword(cel_, group + 2 + (197 - 142) * 8);
        int width = int(word(cel_, p)) + 1, height = int(word(cel_, p + 2)) + 1 - 10;
        OriginalPalette effects(pal_);
        if (width > 0 && width <= 630 && height > 0 && height <= 468)
        {
            if (x_ < 8)
            {
                if (y_ + height + 18 < 479)
                {
                    effects.transform_rect(image, 0, y_ + 8 + height, width, 8, 6);
                }
            }
            else
            {
                effects.transform_rect(image, x_ - 8, y_ + 8, 8, height, 6);
                effects.transform_rect(image, x_ - 8, y_ + 8 + height, width, 8, 6);
            }
        }
        {
            auto rendered = try_sprite(image, 197, x_, y_);
            if (!rendered)
            {
                return s2ne::Result<bool>::failure(rendered.error());
            }
        }
        int text_x = x_ + 88 - int(text_.size()) * (text_.size() > 14 ? 4 : 6);
        if (text_.size() > 14)
        {
            font.draw_small(image, text_, text_x, y_ + 8, 42, true);
        }
        else
        {
            font.draw_original_large(image, text_, text_x, y_ + 8, 42);
        }
        const int left = x_ + 169 + selection_ * 31, right = left + 27, top = y_ + 5, bottom = y_ + 33;
        for (int y = top; y <= bottom; ++y)
        {
            for (int x = left; x <= right; ++x)
            {
                if (x != left && x != right && y != top && y != bottom)
                {
                    continue;
                }
                if (x < 0 || x >= 640 || y < 0 || y >= 480)
                {
                    continue;
                }
                size_t p = (size_t(y) * 640 + x) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    image[p + k] = uint8_t(unsigned(pal_[60 * 3 + k]) * 255 / 63);
                }
                image[p + 3] = 255;
            }
        }
        if (result_)
        {
            {
                auto rendered = try_sprite(image, *result_ ? 198 : 199, x_ + (*result_ ? 169 : 200), y_ + 5);
                if (!rendered)
                {
                    return s2ne::Result<bool>::failure(rendered.error());
                }
            }
        }

        return s2ne::Result<bool>::success(true);
    }
    void draw(std::vector<uint8_t>& image, const OriginalMenu& font) const
    {
        auto result = try_draw(image, font);
        if (!result)
        {
            report_error(result.error());
        }
    }
};
}    //namespace s2ne
