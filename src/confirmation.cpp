#include "confirmation.h"

namespace s2ne
{
Confirmation::Confirmation(const std::filesystem::path& root, const Menu& font, unsigned string_id, int x, int y, bool default_yes) : text_(font.text_bytes(string_id)), x_(x), y_(y), selection_(default_yes ? 0 : 1)
{
    auto read = [](const auto& path)
    {
        return read_shared_resource(path).value();
    };
    cel_ = read(root / "SUPERSP.CEL");
    pal_ = read(root / "SUPERSP.PAL");
    pal_.mutable_at(0) = pal_.mutable_at(1) = pal_.mutable_at(2) = 0;
}

auto Confirmation::toggle() -> void { selection_ ^= 1; }

auto Confirmation::confirm() -> void { result_ = selection_ == 0; }

auto Confirmation::cancel() -> void { result_ = false; }

auto Confirmation::result() const -> std::optional<bool> { return result_; }

auto Confirmation::selection() const -> int { return selection_; }

auto Confirmation::mouse(float x, float y, bool left_click) -> void
{
    if (y < y_ + 4 || y > y_ + 32)
    {
        return;
    }
    int row;
    if (x >= x_ + 169 && x <= x_ + 196) { row = 0; }
    else if (x >= x_ + 200 && x <= x_ + 227) { row = 1; }
    else
    {
        row = -1;
    }

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

auto Confirmation::try_draw(std::vector<uint8_t>& image, const Menu& font) const -> s2ne::Result<bool>
{
    // 431997 calls 475970 with fill=0: outline the embedded O/X artwork.
    auto shadow = draw_original_cel_shadow(image, cel_, pal_, 197, x_, y_);
    if (!shadow) { return shadow; }
    {
        auto rendered = draw_original_cel(image, cel_, pal_, 197, x_, y_, ZeroRun::opaque);
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
            auto rendered = draw_original_cel(image, cel_, pal_, *result_ ? 198 : 199, x_ + (*result_ ? 169 : 200), y_ + 5, ZeroRun::opaque);
            if (!rendered)
            {
                return s2ne::Result<bool>::failure(rendered.error());
            }
        }
    }

    return s2ne::Result<bool>::success(true);
}

auto Confirmation::draw(std::vector<uint8_t>& image, const Menu& font) const -> void
{
    auto result = try_draw(image, font);
    if (!result)
    {
        report_error(result.error());
    }
}
}    //namespace s2ne
