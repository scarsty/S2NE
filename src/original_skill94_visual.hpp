#pragma once
#include "original_skill_visual.hpp"
namespace s2ne
{
// Sword94 only: BIN10 visual84 -> CEL1 SPELLSP85, 46B9A9 mode2.
class OriginalSkill94Visual
{
    std::vector<OriginalSkill32Visual::Frame> frames_;
    size_t frame_{ };
    uint64_t elapsed_{ };
    bool presented_{ };
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }

public:
    static Result<std::vector<OriginalSkill32Visual::Frame>> try_parse(const std::vector<uint8_t>& entry)
    {
        if (entry.size() < 16)
        {
            return Result<std::vector<OriginalSkill32Visual::Frame>>::failure(make_error("Truncated sword94 header"));
        }
        constexpr std::array<unsigned, 8> header{ 15, 0, 0, 107, 132, 0, 0, 5 };
        for (unsigned i = 0; i < 8; ++i)
        {
            if (word(entry, i * 2) != header[i])
            {
                return Result<std::vector<OriginalSkill32Visual::Frame>>::failure(make_error("Unsupported sword94 SPELLSP header"));
            }
        }
        constexpr size_t data = 16 + 15 * 8;
        size_t previous = 0;
        if (entry.size() < data)
        {
            return Result<std::vector<OriginalSkill32Visual::Frame>>::failure(make_error("Truncated sword94 directory"));
        }
        std::vector<OriginalSkill32Visual::Frame> result;
        for (unsigned i = 0; i < 15; ++i)
        {
            size_t off = dword(entry, 16 + i * 8), length = dword(entry, 20 + i * 8);
            if (off != previous || off > entry.size() - data || length > entry.size() - data - off)
            {
                return Result<std::vector<OriginalSkill32Visual::Frame>>::failure(make_error("Invalid sword94 frame extent"));
            }
            auto frame = OriginalSkill32Visual::try_decode({ entry.begin() + data + off, entry.begin() + data + off + length }, 107, 132);
            if (!frame)
            {
                return Result<std::vector<OriginalSkill32Visual::Frame>>::failure(frame.error());
            }
            result.push_back(std::move(frame).value());
            previous = off + length;
        }
        if (previous != entry.size() - data)
        {
            return Result<std::vector<OriginalSkill32Visual::Frame>>::failure(make_error("Unexpected sword94 trailing data"));
        }
        return Result<std::vector<OriginalSkill32Visual::Frame>>::success(std::move(result));
    }
    OriginalSkill94Visual() = default;
    static auto parse(const std::vector<uint8_t>& entry)
    {
        auto result = try_parse(entry);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    static Result<OriginalSkill94Visual> create(const std::filesystem::path& root)
    {
        auto entry = read_spell_entry(root, 94, 84, 233, 35);
        if (!entry)
        {
            return Result<OriginalSkill94Visual>::failure(entry.error());
        }
        auto frames = try_parse(entry.value());
        if (!frames)
        {
            return Result<OriginalSkill94Visual>::failure(frames.error());
        }
        OriginalSkill94Visual result;
        result.frames_ = std::move(frames).value();
        return Result<OriginalSkill94Visual>::success(std::move(result));
    }
    explicit OriginalSkill94Visual(const std::filesystem::path& root)
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
