#include "original_skill94_visual.h"

namespace s2ne
{
auto OriginalSkill94Visual::try_parse(const std::vector<uint8_t>& entry) -> Result<std::vector<OriginalSkill32Visual::Frame>>
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
        frame.value().texture_visual = 85;
        frame.value().texture_frame = i;
        result.push_back(std::move(frame).value());
        previous = off + length;
    }
    if (previous != entry.size() - data)
    {
        return Result<std::vector<OriginalSkill32Visual::Frame>>::failure(make_error("Unexpected sword94 trailing data"));
    }
    return Result<std::vector<OriginalSkill32Visual::Frame>>::success(std::move(result));
}

auto OriginalSkill94Visual::parse(const std::vector<uint8_t>& entry) -> std::vector<s2ne::OriginalSkill32Visual::Frame>
{
    auto result = try_parse(entry);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto OriginalSkill94Visual::create(const std::filesystem::path& root) -> Result<OriginalSkill94Visual>
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

OriginalSkill94Visual::OriginalSkill94Visual(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto OriginalSkill94Visual::frames() const -> const std::vector<s2ne::OriginalSkill32Visual::Frame>& { return frames_; }

auto OriginalSkill94Visual::current() const -> const s2ne::OriginalSkill32Visual::Frame& { return frames_.at(frame_); }

auto OriginalSkill94Visual::frame() const -> size_t { return frame_; }

auto OriginalSkill94Visual::done() const -> bool { return frame_ == frames_.size(); }

auto OriginalSkill94Visual::presented() -> void { presented_ = true; }

auto OriginalSkill94Visual::tick(unsigned ms, bool accelerated) -> void
{
    if (done())
    {
        return;
    }
    elapsed_ += ms;
    if (presented_ && elapsed_ >= (accelerated ? 12u : 50u))
    {
        ++frame_;
        elapsed_ = 0;
        presented_ = false;
    }
}
}    //namespace s2ne
