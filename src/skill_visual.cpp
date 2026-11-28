#include "skill_visual.h"

namespace s2ne
{
auto Skill32Visual::try_decode(const std::vector<uint8_t>& bytes, unsigned expected_width, unsigned expected_height) -> Result<Frame>
{
    if (bytes.size() < 4)
    {
        return Result<Frame>::failure(make_error("Truncated original skill frame"));
    }
    int w = int(int16_t(word(bytes, 0))) + 1, h = int(int16_t(word(bytes, 2))) + 1;
    if (w != int(expected_width) || h != int(expected_height))
    {
        return Result<Frame>::failure(make_error("Unsupported original spell33 frame extent"));
    }
    Frame result{ unsigned(w), unsigned(h), {} };
    size_t size = size_t(w) * h;
    result.pixels.reserve(size);
    for (size_t p = 4; p < bytes.size();)
    {
        unsigned code = bytes.at(p++), n = code ? code & 127 : 1;
        if (n > size - result.pixels.size())
        {
            return Result<Frame>::failure(make_error("Original skill run overflow"));
        }
        if (!code)
        {
            result.pixels.push_back(0);
            continue;
        }
        unsigned source = code & 128 ? n : 1;
        if (source > bytes.size() - p)
        {
            return Result<Frame>::failure(make_error("Truncated original skill run"));
        }
        if (code & 128)
        {
            while (n--)
            {
                result.pixels.push_back(bytes.at(p++));
            }
        }
        else
        {
            auto color = bytes.at(p++);
            result.pixels.insert(result.pixels.end(), n, color);
        }
    }
    if (result.pixels.size() != size)
    {
        return Result<Frame>::failure(make_error("Incomplete original skill frame"));
    }
    return Result<Frame>::success(std::move(result));
}

auto Skill32Visual::try_parse(const std::vector<uint8_t>& entry) -> Result<std::vector<Frame>>
{
    // No extrapolation to palette-bearing, ordered/repeating or other anchor modes.
    const std::array<unsigned, 8> header{ 23, 0, 0, 63, 272, 0, 0, 5 };
    if (entry.size() < 16)
    {
        return Result<std::vector<Frame>>::failure(make_error("Truncated original SPELLSP header"));
    }
    for (unsigned i = 0; i < header.size(); ++i)
    {
        if (word(entry, i * 2) != header[i])
        {
            return Result<std::vector<Frame>>::failure(make_error("Unsupported original SPELLSP layout"));
        }
    }
    const size_t data = 16 + 23 * 8;
    if (entry.size() < data)
    {
        return Result<std::vector<Frame>>::failure(make_error("Truncated original SPELLSP directory"));
    }
    std::vector<Frame> frames;
    size_t previous = 0;
    for (unsigned i = 0; i < 23; ++i)
    {
        size_t offset = dword(entry, 16 + i * 8), length = dword(entry, 20 + i * 8);
        if (offset != previous || offset > entry.size() - data || length > entry.size() - data - offset)
        {
            return Result<std::vector<Frame>>::failure(make_error("Invalid original SPELLSP frame directory"));
        }
        auto decoded = try_decode({ entry.begin() + data + offset, entry.begin() + data + offset + length });
        if (!decoded)
        {
            return Result<std::vector<Frame>>::failure(decoded.error());
        }
        decoded.value().texture_visual = 33;
        decoded.value().texture_frame = i;
        frames.push_back(std::move(decoded).value());
        previous = offset + length;
    }
    if (previous != entry.size() - data)
    {
        return Result<std::vector<Frame>>::failure(make_error("Unexpected original SPELLSP trailing data"));
    }
    return Result<std::vector<Frame>>::success(std::move(frames));
}

auto Skill32Visual::decode(const std::vector<uint8_t>& bytes, unsigned width, unsigned height) -> Frame
{
    auto result = try_decode(bytes, width, height);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Skill32Visual::parse(const std::vector<uint8_t>& entry) -> std::vector<Frame>
{
    auto result = try_parse(entry);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Skill32Visual::create(const std::filesystem::path& root) -> Result<Skill32Visual>
{
    auto entry = read_spell_entry(root, 32, 32, 140, 45);
    if (!entry)
    {
        return Result<Skill32Visual>::failure(entry.error());
    }
    auto frames = try_parse(entry.value());
    if (!frames)
    {
        return Result<Skill32Visual>::failure(frames.error());
    }
    Skill32Visual result;
    result.frames_ = std::move(frames).value();
    result.duration_ = 50;
    return Result<Skill32Visual>::success(std::move(result));
}

auto Skill32Visual::create_curse33(const std::filesystem::path& root) -> Result<Skill32Visual>
{
    auto loaded = read_spell_entry(root, 33, 33, 0, 0);
    if (!loaded) { return Result<Skill32Visual>::failure(loaded.error()); }
    const auto& entry = loaded.value();
    ResourceReader reader(entry);
    constexpr std::array<unsigned, 8> header{ 21, 0, 0, 71, 344, 7, 0, 5 };
    for (unsigned i = 0; i < 8; ++i)
    {
        if (reader.number(i * 2) != header[i]) { return Result<Skill32Visual>::failure(make_error("Invalid curse33 header")); }
    }
    if (!reader.extent(16, 14 + 21 * 8)) { return Result<Skill32Visual>::failure(reader.error()); }
    Skill32Visual out;
    out.curse_ = true;
    out.duration_ = 50;
    out.amounts_.fill(-1);
    for (unsigned i = 0; i < 7; ++i) { out.amounts_[entry[16 + i]] = entry[23 + i]; }
    size_t data = 30 + 21 * 8, previous = 0;
    for (unsigned i = 0; i < 21; ++i)
    {
        size_t offset = reader.number(30 + i * 8, 4), length = reader.number(34 + i * 8, 4);
        if (!reader || offset != previous || data > entry.size() || offset > entry.size() - data || length > entry.size() - data - offset)
        {
            return Result<Skill32Visual>::failure(make_error("Invalid curse33 directory"));
        }
        auto frame = try_decode({ entry.begin() + data + offset, entry.begin() + data + offset + length }, 71, 344);
        if (!frame) { return Result<Skill32Visual>::failure(frame.error()); }
        frame.value().texture_visual = 34;
        frame.value().texture_frame = i;
        out.frames_.push_back(std::move(frame).value());
        previous = offset + length;
    }
    if (previous != entry.size() - data) { return Result<Skill32Visual>::failure(make_error("Invalid curse33 trailing bytes")); }
    return Result<Skill32Visual>::success(std::move(out));
}

auto Skill32Visual::curse() const -> bool { return curse_; }

auto Skill32Visual::amount(unsigned color) const -> int { return curse_ ? amounts_[color] : -1; }

Skill32Visual::Skill32Visual(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto Skill32Visual::frames() const -> const std::vector<Frame>& { return frames_; }

auto Skill32Visual::current() const -> const Frame& { return frames_.at(frame_); }

auto Skill32Visual::frame() const -> size_t { return frame_; }

auto Skill32Visual::done() const -> bool { return frame_ == frames_.size(); }

auto Skill32Visual::duration_ms() const -> unsigned { return duration_; }

auto Skill32Visual::presented() -> void { presented_ = true; }

auto Skill32Visual::tick(unsigned milliseconds, bool accelerated) -> void
{
    if (done())
    {
        return;
    }
    elapsed_ += milliseconds;
    const unsigned delay = accelerated ? std::max(1u, duration_ / 4) : duration_;
    if (presented_ && elapsed_ >= delay)
    {
        ++frame_;
        elapsed_ = 0;
        presented_ = false;
    }
}

auto Skill32Visual::tint(std::vector<uint8_t>& indexed, const Palette& palette) -> void
{
    std::array<uint8_t, 256> colors{};
    for (unsigned i = 1; i < 256; ++i)
    {
        colors[i] = palette.blend_index(i, 140, 45);
    }
    for (auto& c : indexed)
    {
        if (c)
        {
            c = colors[c];
        }
    }
}
}    //namespace s2ne
