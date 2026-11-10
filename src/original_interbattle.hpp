#pragma once
#include "original_menu.hpp"
#include "title_image.hpp"
#include <functional>
#include <optional>

namespace s2ne
{
class OriginalInterbattle
{
    OriginalMenu font_;
    std::vector<uint8_t> background_, palette_, indices_;
    int selected_{ 1 }, pointer_x_{ 298 }, pointer_y_{ 253 };
    bool pending_{ }, closed_{ };
    static std::vector<uint8_t> read(const std::filesystem::path& p)
    {
        std::ifstream f(p, std::ios::binary);
        if (!f)
        {
            throw std::runtime_error("Missing original interbattle resource");
        }
        return { std::istreambuf_iterator<char>(f), { } };
    }
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    void caption(std::vector<uint8_t>& image, unsigned id, int x, int y) const
    {
        const auto text = font_.text_bytes(id);
        // Font helper uses global colors; the PCX menu owns its palette.
        for (unsigned ink : { 2u, 3u })
        {
            std::vector<uint8_t> mask(image.size());
            int delta = ink == 2 ? 1 : 0;
            font_.draw_small(mask, text, x + delta, y + delta, ink);
            for (size_t p = 0; p < mask.size(); p += 4)
            {
                if (mask[p + 3])
                {
                    for (unsigned k = 0; k < 3; ++k)
                    {
                        image[p + k] = opening_palette_channel(uint8_t(palette_[ink * 3 + k] >> 2));
                    }
                    image[p + 3] = 255;
                }
            }
        }
    }

public:
    struct Rect
    {
        int left, top, right, bottom;
    };
    struct Request
    {
        unsigned option{ }, label{ }, address{ };
    };
    enum class Result
    {
        unsupported,
        returned,
        enter_battle,
        exit_to_title
    };
    // 4EA600 begins with screen title88. Four options use subsequent entries.
    static constexpr std::array<unsigned, 4> labels{ 89, 90, 91, 92 };
    static constexpr std::array<unsigned, 4> targets{ 0x410EEE, 0x410DC0, 0x41115D, 0x416333 };
    static constexpr std::array<Rect, 4> rectangles{ { { 226, 211, 370, 296 }, { 97, 277, 206, 366 }, { 421, 272, 584, 402 }, { 79, 419, 296, 479 } } };
    explicit OriginalInterbattle(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one, unsigned resource = 17) : font_(root)
    {
        if (resource != 17 && resource != 14 && resource != 15 && resource != 13)
        {
            throw std::invalid_argument("Unsupported original interbattle background");
        }
        auto dat = OriginalDiscResources(root).dat_entry(disc, resource);
        size_t offset = 0, size = dat.size();
        if (offset > dat.size() || size > dat.size() - offset || size < 897)
        {
            throw std::runtime_error("Invalid hub PCX17");
        }
        if (dat.at(offset) != 10 || dat.at(offset + 2) != 1 || dat.at(offset + 3) != 8 || dat.at(offset + 65) != 1 || word(dat, offset + 8) != 639 || word(dat, offset + 10) != 479)
        {
            throw std::runtime_error("Unsupported hub PCX17");
        }
        unsigned stride = word(dat, offset + 66);
        if (stride < 640 || stride > 4096)
        {
            throw std::runtime_error("Invalid hub stride");
        }
        size_t colors = offset + size - 768;
        if (dat.at(colors - 1) != 12)
        {
            throw std::runtime_error("Missing hub palette");
        }
        palette_.assign(dat.begin() + colors, dat.begin() + colors + 768);
        background_.resize(640 * 480 * 4);
        indices_.resize(640 * 480);
        size_t p = offset + 128, pixel = 0, total = size_t(stride) * 480;
        while (pixel < total)
        {
            if (p >= colors - 1)
            {
                throw std::runtime_error("Truncated hub PCX");
            }
            unsigned value = dat.at(p++), n = 1;
            if ((value & 192) == 192)
            {
                n = value & 63;
                if (p >= colors - 1)
                {
                    throw std::runtime_error("Truncated hub run");
                }
                value = dat.at(p++);
            }
            if (!n || n > total - pixel)
            {
                throw std::runtime_error("Invalid hub run");
            }
            while (n--)
            {
                size_t x = pixel % stride, y = pixel / stride;
                ++pixel;
                if (x >= 640)
                {
                    continue;
                }
                indices_[y * 640 + x] = uint8_t(value);
                size_t q = (y * 640 + x) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    background_[q + k] = opening_palette_channel(uint8_t(palette_[value * 3 + k] >> 2));
                }
                background_[q + 3] = 255;
            }
        }
    }
    const std::vector<uint8_t>& background() const { return background_; }
    std::vector<uint8_t> indexed_background(const std::vector<uint8_t>& palette) const
    {
        if (palette.size() < 768)
        {
            throw std::invalid_argument("Original indexed background palette");
        }
        auto image = background_;
        for (size_t i = 0; i < indices_.size(); ++i)
        {
            for (unsigned k = 0; k < 3; ++k)
            {
                image[i * 4 + k] = uint8_t(unsigned(palette[indices_[i] * 3 + k]) * 255 / 63);
            }
        }
        return image;
    }
    int selected() const { return selected_; }
    bool closed() const { return closed_; }
    bool pending() const { return pending_; }
    static int hit(int x, int y)
    {
        for (unsigned i = 0; i < rectangles.size(); ++i)
        {
            auto r = rectangles[i];
            if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom)
            {
                return int(i) + 1;
            }
        }
        return 0;
    }
    void mouse(int x, int y, bool left_click)
    {
        if (closed_ || pending_)
        {
            return;
        }
        pointer_x_ = x;
        pointer_y_ = y;
        int row = hit(x, y);
        if (row)
        {
            selected_ = row;
            if (left_click)
            {
                pending_ = true;
            }
        }
    }
    void scan(unsigned code)
    {
        if (closed_ || pending_)
        {
            return;
        }
        // 410A3F: Left AND Right AND Tab all advance; Escape is disabled at hub.
        if (code == 0x4B || code == 0x4D || code == 0x0F)
        {
            selected_ = selected_ % 4 + 1;
            auto r = rectangles[size_t(selected_ - 1)];
            pointer_x_ = (r.left + r.right) / 2;
            pointer_y_ = (r.top + r.bottom) / 2;
        }
        else if (code == 0x1C || code == 0x39)
        {
            pending_ = true;
        }
    }
    std::optional<Request> request() const
    {
        if (!pending_ || closed_)
        {
            return std::nullopt;
        }
        size_t row = size_t(selected_ - 1);
        return Request{ unsigned(selected_), labels[row], targets[row] };
    }
    // Execute the original subpage via adapter. Unsupported leaves hub pending;
    // it must never silently grant lodging, items, promotion or skip deployment.
    bool dispatch(const std::function<Result(Request)>& adapter)
    {
        auto req = request();
        if (!req || !adapter)
        {
            return false;
        }
        Result result = adapter(*req);
        if (result == Result::unsupported)
        {
            return false;
        }
        if (result == Result::enter_battle && req->option != 4)
        {
            return false;
        }
        pending_ = false;
        closed_ = result == Result::enter_battle || result == Result::exit_to_title;
        return true;
    }
    std::vector<uint8_t> draw(bool all_captions = false) const
    {
        auto image = background_;
        if (all_captions)
        {
            for (size_t i = 0; i < labels.size(); ++i)
            {
                auto r = rectangles[i];
                auto t = font_.text_bytes(labels[i]);
                int x = std::clamp((r.left + r.right) / 2 - int(t.size()) * 4, 0, 639 - int(t.size()) * 8);
                caption(image, labels[i], x, (r.top + r.bottom) / 2);
            }
        }
        else
        {
            auto text = font_.text_bytes(labels[size_t(selected_ - 1)]);
            int x = std::clamp(pointer_x_ - int(text.size()) * 4, 0, 639 - int(text.size()) * 8);
            caption(image, labels[size_t(selected_ - 1)], x, std::max(30, pointer_y_ - 30));
        }
        return image;
    }
};
}    //namespace s2ne
