#include "original_scroll_text.h"

namespace s2ne
{
OriginalScrollText::OriginalScrollText(const std::filesystem::path& root, unsigned background, unsigned text, OriginalDisc disc) : font_(root)
{
    auto pal = read_resource(root / "SUPERSP.PAL").value();
    auto bin = read_shared_resource(root / "SUPERSP.BIN").value();
    size_t base = dword(pal, 864) - 13, size = dword(pal, 868) - 13;
    if (base > bin.size() || size > bin.size() - base) { fatal("Invalid scroll BIN3 extent"); }
    std::vector<uint8_t> strings(bin.begin() + base, bin.begin() + base + size);
    if (!text || text > word(strings, 0)) { fatal("Invalid scroll text record"); }
    size_t p = dword(strings, 4 + (text - 1) * 6);
    rows_.emplace_back();
    unsigned units = 0;
    while (strings.at(p) && strings.at(p) != 10)
    {
        unsigned c = strings.at(p++);
        if (c == '*')
        {
            rows_.emplace_back();
            units = 0;
            continue;
        }
        rows_.back().push_back(uint8_t(c));
        if (c > 128)
        {
            rows_.back().push_back(strings.at(p++));
            units += 2;
        }
        else
        {
            ++units;
        }
        if (units >= 42)
        {
            rows_.emplace_back();
            units -= 42;
        }
    }
    remaining_ = int(rows_.size() * 28 + 2);
    for (unsigned c = 0; c < 3; ++c) { ink_[c] = uint8_t(unsigned(pal.at(254 * 3 + c)) * 255 / 63); }
    background_.assign(640 * 480 * 4, 0);
    for (size_t i = 3; i < background_.size(); i += 4) { background_[i] = 255; }
    if (background)
    {
        background_ = OriginalEnding::try_pcx(root, background, disc).value();
        background_.resize(640 * 480 * 4);
        if (!texture_resources.active())
        {
            auto pcx = OriginalDiscResources(root).dat_entry(disc, background);
            for (unsigned c = 0; c < 3; ++c) { ink_[c] = pcx.at(pcx.size() - 768 + 255 * 3 + c); }
        }
    }
}

auto OriginalScrollText::done() const -> bool { return done_; }

auto OriginalScrollText::cancel() -> void { done_ = true; }

auto OriginalScrollText::tick(unsigned milliseconds) -> void
{
    elapsed_ += milliseconds;
    while (!done_ && elapsed_ >= 12)
    {
        elapsed_ -= 12;
        if (remaining_ <= 94 && visible_ <= 94) { --visible_; }
        else if (visible_ < 94) { ++visible_; }
        visible_ = std::clamp(visible_, 0, 94);
        if (top_ > 386) { --top_; }
        else
        {
            ++source_;
            remaining_ = std::max(0, remaining_ - 1);
        }
        if (!remaining_ && !visible_) { done_ = true; }
    }
}

auto OriginalScrollText::draw() const -> std::vector<uint8_t>
{
    auto image = background_;
    auto layer = std::vector<uint8_t>(image.size());
    for (size_t row = 0; row < rows_.size(); ++row)
    {
        int y = top_ + int(row * 28) - source_;
        if (y < top_ - 28 || y > top_ + visible_) { continue; }
        font_.draw_original_wrapped(layer, rows_[row], 25, y, 254, 1, 21, 28);
    }
    for (int y = top_; y < std::min(480, top_ + visible_); ++y)
    {
        for (int x = 0; x < 640; ++x)
        {
            size_t p = (size_t(y) * 640 + x) * 4;
            if (!layer[p + 3]) { continue; }
            const unsigned alpha = layer[p + 3];
            for (unsigned c = 0; c < 3; ++c) { image[p + c] = uint8_t((unsigned(ink_[c]) * alpha + unsigned(image[p + c]) * (255 - alpha) + 127) / 255); }
        }
    }
    return image;
}
}    //namespace s2ne
