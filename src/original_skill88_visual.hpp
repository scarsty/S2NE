#pragma once
#include "original_skill_visual.hpp"
namespace s2ne
{
class OriginalSkill88Visual
{
    std::vector<OriginalSkill32Visual::Frame> frames_;
    std::array<int, 256> amounts_{ };
    size_t frame_{ };
    uint64_t elapsed_{ };
    bool presented_{ };
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }

public:
    OriginalSkill88Visual() = default;
    static Result<OriginalSkill88Visual> create(const std::filesystem::path& root)
    {
        auto loaded = read_spell_entry(root, 88, 78, 233, 50);
        if (!loaded)
        {
            return Result<OriginalSkill88Visual>::failure(loaded.error());
        }
        const auto& entry = loaded.value();
        OriginalSkill88Visual result;
        result.amounts_.fill(-1);
        if (entry.size() < 28)
        {
            return Result<OriginalSkill88Visual>::failure(make_error("Truncated88 header"));
        }
        constexpr std::array<unsigned, 8> header{ 35, 89, 124, 175, 124, 6, 0, 5 };
        for (unsigned i = 0; i < 8; ++i)
        {
            if (word(entry, i * 2) != header[i])
            {
                return Result<OriginalSkill88Visual>::failure(make_error("Unsupported88 SPELLSP layout"));
            }
        }
        // 463EA6 maps six source indices to their independent blend percentages.
        for (unsigned i = 0; i < 6; ++i)
        {
            result.amounts_[entry.at(16 + i)] = entry.at(22 + i);
        }
        size_t data = 28 + 35 * 8, previous = 0;
        if (entry.size() < data)
        {
            return Result<OriginalSkill88Visual>::failure(make_error("Truncated88 directory"));
        }
        for (unsigned i = 0; i < 35; ++i)
        {
            size_t off = dword(entry, 28 + i * 8), size = dword(entry, 32 + i * 8);
            if (off != previous || off > entry.size() - data || size > entry.size() - data - off)
            {
                return Result<OriginalSkill88Visual>::failure(make_error("Invalid88 frame extent"));
            }
            auto frame = OriginalSkill32Visual::try_decode({ entry.begin() + data + off, entry.begin() + data + off + size }, 175, 124);
            if (!frame)
            {
                return Result<OriginalSkill88Visual>::failure(frame.error());
            }
            result.frames_.push_back(std::move(frame).value());
            previous = off + size;
        }
        if (previous != entry.size() - data)
        {
            return Result<OriginalSkill88Visual>::failure(make_error("Unexpected88 trailing bytes"));
        }
        return Result<OriginalSkill88Visual>::success(std::move(result));
    }
    explicit OriginalSkill88Visual(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    const auto& frames() const { return frames_; }
    const auto& current() const { return frames_.at(frame_); }
    int amount(unsigned color) const { return amounts_.at(color); }
    size_t frame() const { return frame_; }
    bool done() const { return frame_ == frames_.size(); }
    void presented() { presented_ = true; }
    void tick(unsigned ms)
    {
        if (done())
        {
            return;
        }
        elapsed_ += ms;
        if (presented_ && elapsed_ >= 50)
        {
            ++frame_;
            elapsed_ = 0;
            presented_ = false;
        }
    }
};
}    //namespace s2ne
