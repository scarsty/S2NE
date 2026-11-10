#pragma once
#include "original_menu.hpp"
#include "original_save.hpp"
#include "title_image.hpp"
#include <algorithm>
namespace s2ne
{
class OriginalWorldmap
{
    struct Icon
    {
        unsigned width{ }, height{ };
        std::vector<uint8_t> pixels;
    };
    std::vector<uint8_t> pixels_, palette_, towns_;
    std::array<Icon, 2> icons_;
    unsigned width_{ }, height_{ }, current_{ }, destination_{ }, hovered_{ };
    int x_{ }, y_{ };
    bool closed_{ };
    int pointer_x_{ 320 }, pointer_y_{ 240 };
    std::optional<std::pair<int, int>> pending_scroll_;
    unsigned last_scroll_{ }, delay_{ };
    bool waiting_{ };
    bool presented_{ };
    bool presentation_{ }, entering_{ }, leaving_{ };
    unsigned fade_{ 63 };
    bool transition() const { return presentation_ && (leaving_ || entering_); }
    uint8_t channel(unsigned ink, unsigned component) const
    {
        auto dac = uint8_t(palette_[ink * 3 + component] >> 2);
        if (presentation_ && leaving_)
        {
            return uint8_t(unsigned(opening_palette_channel(dac, 63)) * (fade_ << 2) / 255);
        }
        return opening_palette_channel(dac, presentation_ ? fade_ : 60);
    }
    void close()
    {
        if (presentation_)
        {
            leaving_ = true;
            fade_ = 63;
            presented_ = false;
        }
        else
        {
            closed_ = true;
        }
    }
    static unsigned word(const auto& b, size_t p) { return unsigned(b[p]) | (unsigned(b[p + 1]) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    void center(int x, int y)
    {
        x_ = std::clamp(x - 320, 0, int(width_) - 640);
        y_ = std::clamp(y - 240, 0, int(height_) - 480);
    }
    void request_scroll(int dx, int dy)
    {
        if (presentation_)
        {
            pending_scroll_ = std::pair{ dx, dy };
        }
        else
        {
            scroll(dx, dy);
        }
    }
    void scroll(int dx, int dy)
    {
        x_ += dx < 0 && x_ > 0 ? dx : dx > 0 && x_ < int(width_) - 640 ? dx :
                                                                         0;
        y_ += dy < 0 && y_ > 0 ? dy : dy > 0 && y_ < int(height_) - 480 ? dy :
                                                                          0;
        x_ = x_ < 0 ? 0 : x_ >= int(width_) - 640 ? std::max(0, int(width_) - 641) :
                                                    x_;
        y_ = y_ < 0 ? 0 : y_ >= int(height_) - 480 ? std::max(0, int(height_) - 481) :
                                                     y_;
        hovered_ = 0;
    }

public:
    static Result<OriginalWorldmap> create(const std::filesystem::path& root, OriginalDisc domain, std::vector<uint8_t> towns, const OriginalSave::State& state)
    {
        auto image = OriginalDiscResources(root).try_dat_entry(domain, 11);
        auto archive = OriginalDiscResources(root).try_dat_entry(domain, 7);
        if (!image)
        {
            return Result<OriginalWorldmap>::failure(image.error());
        }
        if (!archive)
        {
            return Result<OriginalWorldmap>::failure(archive.error());
        }
        const auto& pcx = image.value();
        if (pcx.size() < 897 || pcx[0] != 10 || pcx[2] != 1 || pcx[3] != 8 || pcx[65] != 1 || pcx[pcx.size() - 769] != 12)
        {
            return Result<OriginalWorldmap>::failure(make_error("41020B invalid original world PCX11"));
        }
        OriginalWorldmap result;
        result.width_ = word(pcx, 8) + 1;
        result.height_ = word(pcx, 10) + 1;
        unsigned stride = word(pcx, 66);
        if (result.width_ < 640 || result.height_ < 480 || result.width_ > 8192 || result.height_ > 8192 || stride < result.width_)
        {
            return Result<OriginalWorldmap>::failure(make_error("41020B invalid world dimensions"));
        }
        result.palette_.assign(pcx.end() - 768, pcx.end());
        result.pixels_.resize(size_t(result.width_) * result.height_);
        size_t p = 128, pixel = 0, total = size_t(stride) * result.height_;
        while (pixel < total)
        {
            if (p >= pcx.size() - 769)
            {
                return Result<OriginalWorldmap>::failure(make_error("Truncated world PCX"));
            }
            unsigned ink = pcx[p++], count = 1;
            if ((ink & 192) == 192)
            {
                count = ink & 63;
                if (p >= pcx.size() - 769)
                {
                    return Result<OriginalWorldmap>::failure(make_error("Truncated world run"));
                }
                ink = pcx[p++];
            }
            if (!count || count > total - pixel)
            {
                return Result<OriginalWorldmap>::failure(make_error("Invalid world run"));
            }
            while (count--)
            {
                if (pixel % stride < result.width_)
                {
                    result.pixels_[(pixel / stride) * result.width_ + pixel % stride] = uint8_t(ink);
                }
                ++pixel;
            }
        }
        const auto& group = archive.value();
        if (group.size() < 50 || word(group, 0) < 6)
        {
            return Result<OriginalWorldmap>::failure(make_error("426971 world markers missing"));
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            size_t offset = dword(group, 2 + (4 + i) * 8);
            if (offset > group.size() || group.size() - offset < 5)
            {
                return Result<OriginalWorldmap>::failure(make_error("World marker header"));
            }
            auto& icon = result.icons_[i];
            if (group[offset] != 0)
            {
                return Result<OriginalWorldmap>::failure(make_error("Unsupported world marker encoding"));
            }
            icon.width = word(group, offset + 1) + 1;
            icon.height = word(group, offset + 3) + 1;
            if (icon.width > 640 || icon.height > 480)
            {
                return Result<OriginalWorldmap>::failure(make_error("World marker dimensions"));
            }
            size_t cursor = offset + 5;
            for (unsigned row = 0; row < icon.height; ++row)
            {
                for (unsigned x = 0; x < icon.width;)
                {
                    if (cursor >= group.size())
                    {
                        return Result<OriginalWorldmap>::failure(make_error("World marker run"));
                    }
                    unsigned op = group[cursor++], count = op ? op & 127 : 1, ink = 0;
                    if (!count || count > icon.width - x)
                    {
                        return Result<OriginalWorldmap>::failure(make_error("World marker row"));
                    }
                    if (op && !(op & 128))
                    {
                        if (cursor >= group.size())
                        {
                            return Result<OriginalWorldmap>::failure(make_error("World marker ink"));
                        }
                        ink = group[cursor++];
                    }
                    while (count--)
                    {
                        if ((op & 128) && cursor >= group.size())
                        {
                            return Result<OriginalWorldmap>::failure(make_error("World marker literal"));
                        }
                        icon.pixels.push_back(uint8_t(op & 128 ? group[cursor++] : ink));
                        ++x;
                    }
                }
            }
        }
        if (towns.size() < 2 || size_t(word(towns, 0)) * 14 > towns.size() - 2)
        {
            return Result<OriginalWorldmap>::failure(make_error("World town records"));
        }
        for (unsigned i = 0; i < word(towns, 0); ++i)
        {
            size_t name = dword(towns, 2 + i * 14);
            if (name >= towns.size() || std::find(towns.begin() + name, towns.end(), uint8_t(0)) == towns.end())
            {
                return Result<OriginalWorldmap>::failure(make_error("World town name bounds"));
            }
        }
        result.towns_ = std::move(towns);
        result.delay_ = dword(state.globals_4f7cd0, 32);
        result.current_ = word(state.globals_4f7cd0, 24);
        result.destination_ = word(state.globals_4f7cd0, 16);
        if (!result.current_ || result.current_ > word(result.towns_, 0) || !result.destination_ || result.destination_ > word(result.towns_, 0))
        {
            return Result<OriginalWorldmap>::failure(make_error("World current/destination town"));
        }
        size_t record = 2 + (result.current_ - 1) * 14;
        result.center(word(result.towns_, record + 6), word(result.towns_, record + 8));
        return Result<OriginalWorldmap>::success(std::move(result));
    }
    void enable_presentation_timing()
    {
        presentation_ = true;
        entering_ = true;
        fade_ = 0;
        presented_ = false;
    }
    void frame_presented() { presented_ = true; }
    unsigned fade_level() const { return fade_; }
    void tick(unsigned now)
    {
        if (!presented_ || closed_)
        {
            return;
        }
        if (transition())
        {
            if (leaving_ && fade_ == 0)
            {
                closed_ = true;
            }
            else if (leaving_)
            {
                --fade_;
            }
            else if (fade_ == 63)
            {
                entering_ = false;
            }
            else
            {
                ++fade_;
            }
            presented_ = false;
            return;
        }
        if (waiting_ && unsigned(std::abs(int(now) - int(last_scroll_))) < delay_)
        {
            return;
        }
        waiting_ = false;
        int dx = pointer_x_ < 5 ? -4 : pointer_x_ > 635 ? 4 :
                                                          0;
        int dy = pointer_y_ < 5 ? -4 : pointer_y_ > 475 ? 4 :
                                                          0;
        if (pending_scroll_)
        {
            dx = pending_scroll_->first;
            dy = pending_scroll_->second;
            pending_scroll_.reset();
        }
        if (!dx && !dy)
        {
            return;
        }
        auto previous = viewport();
        scroll(dx, dy);
        if (previous != viewport())
        {
            last_scroll_ = now;
            waiting_ = true;
            presented_ = false;
        }
    }
    bool closed() const { return closed_; }
    unsigned hovered() const { return hovered_; }
    unsigned scroll_delay() const { return delay_; }
    std::pair<int, int> viewport() const { return { x_, y_ }; }
    void scan(unsigned code)
    {
        if (transition())
        {
            return;
        }
        if (code == 1 || code == 0x1C)
        {
            close();
        }
        else if (code == 0x48)
        {
            request_scroll(0, -4);
        }
        else if (code == 0x50)
        {
            request_scroll(0, 4);
        }
        else if (code == 0x4B)
        {
            request_scroll(-4, 0);
        }
        else if (code == 0x4D)
        {
            request_scroll(4, 0);
        }
        else if (code == 0x47)
        {
            request_scroll(-4, -4);
        }
        else if (code == 0x4F)
        {
            request_scroll(-4, 4);
        }
        else if (code == 0x49)
        {
            request_scroll(4, -4);
        }
        else if (code == 0x51)
        {
            request_scroll(4, 4);
        }
    }
    void mouse(int x, int y, bool left, bool right)
    {
        if (transition())
        {
            return;
        }
        pointer_x_ = x;
        pointer_y_ = y;
        if (right && (x < 60 || x > 580 || y < 40 || y > 440))
        {
            return;
        }
        if (right)
        {
            close();
            return;
        }
        if (left)
        {
            center(x_ + x, y_ + y);
            hovered_ = 0;
            return;
        }
        hovered_ = 0;
        for (unsigned i = 0; i < word(towns_, 0); ++i)
        {
            size_t p = 2 + i * 14;
            int tx = word(towns_, p + 6), ty = word(towns_, p + 8);
            if (x_ + x >= tx - 20 && x_ + x <= tx + 20 && y_ + y >= ty - 20 && y_ + y <= ty + 20)
            {
                hovered_ = i + 1;
                break;
            }
        }
    }
    std::vector<uint8_t> draw(const OriginalMenu& font) const
    {
        std::vector<uint8_t> image(640 * 480 * 4);
        auto put = [&](int x, int y, unsigned ink)
        {
            if (x < 0 || x >= 640 || y < 0 || y >= 480)
            {
                return;
            }
            size_t p = (size_t(y) * 640 + x) * 4;
            for (unsigned k = 0; k < 3; ++k)
            {
                image[p + k] = channel(ink, k);
            }
            image[p + 3] = 255;
        };
        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                put(x, y, pixels_[size_t(y + y_) * width_ + x + x_]);
            }
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            unsigned town = i ? destination_ : current_;
            size_t p = 2 + (town - 1) * 14;
            int left = int(word(towns_, p + 6)) - x_ - 7, top = int(word(towns_, p + 8)) - y_ - (i ? 30 : 25);
            const auto& icon = icons_[i ? 0 : 1];
            for (unsigned y = 0; y < icon.height; ++y)
            {
                for (unsigned x = 0; x < icon.width; ++x)
                {
                    if (unsigned ink = icon.pixels[size_t(y) * icon.width + x])
                    {
                        put(left + x, top + y, ink);
                    }
                }
            }
        }
        if (hovered_)
        {
            size_t record = 2 + (hovered_ - 1) * 14, p = dword(towns_, record);
            if (p < towns_.size())
            {
                auto end = std::find(towns_.begin() + p, towns_.end(), uint8_t(0));
                std::vector<uint8_t> text(towns_.begin() + p, end);
                int x = std::clamp(int(word(towns_, record + 6)) - x_ - int(text.size()) * 4, 0, std::max(0, 639 - int(text.size()) * 8));
                int y = int(word(towns_, record + 8)) - y_ - 24;
                if (y < 0)
                {
                    y = 24;
                }
                for (unsigned ink : { 3u, 9u })
                {
                    std::vector<uint8_t> mask(image.size());
                    int delta = ink == 3 ? 1 : 0;
                    font.draw_small(mask, text, x + delta, y + delta, ink);
                    for (size_t p = 0; p < mask.size(); p += 4)
                    {
                        if (mask[p + 3])
                        {
                            for (unsigned k = 0; k < 3; ++k)
                            {
                                image[p + k] = channel(ink, k);
                            }
                            image[p + 3] = 255;
                        }
                    }
                }
            }
        }
        return image;
    }
};
}    //namespace s2ne
