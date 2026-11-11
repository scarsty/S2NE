#pragma once
#include "original_palette.hpp"
#include "spell_resource.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>

namespace s2ne
{
// Bounded 463D6D/463EA6 reader: spell32 selects CEL1 SPELLSP entry33.
class OriginalSkill32Visual
{
public:
    struct Frame
    {
        unsigned width{ }, height{ };
        std::vector<uint8_t> pixels;
    };

private:
    std::vector<Frame> frames_;
    unsigned duration_{ };
    std::array<int, 256> amounts_{ };
    bool curse_{ };
    size_t frame_{ };
    uint64_t elapsed_{ };
    bool presented_{ };
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }

public:
    static Result<Frame> try_decode(const std::vector<uint8_t>& bytes, unsigned expected_width = 63, unsigned expected_height = 272)
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
        Frame result{ unsigned(w), unsigned(h), { } };
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
    static Result<std::vector<Frame>> try_parse(const std::vector<uint8_t>& entry)
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
            frames.push_back(std::move(decoded).value());
            previous = offset + length;
        }
        if (previous != entry.size() - data)
        {
            return Result<std::vector<Frame>>::failure(make_error("Unexpected original SPELLSP trailing data"));
        }
        return Result<std::vector<Frame>>::success(std::move(frames));
    }
    static Frame decode(const std::vector<uint8_t>& bytes, unsigned width = 63, unsigned height = 272)
    {
        auto result = try_decode(bytes, width, height);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    static std::vector<Frame> parse(const std::vector<uint8_t>& entry)
    {
        auto result = try_parse(entry);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    OriginalSkill32Visual() = default;
    static Result<OriginalSkill32Visual> create(const std::filesystem::path& root)
    {
        auto entry = read_spell_entry(root, 32, 32, 140, 45);
        if (!entry)
        {
            return Result<OriginalSkill32Visual>::failure(entry.error());
        }
        auto frames = try_parse(entry.value());
        if (!frames)
        {
            return Result<OriginalSkill32Visual>::failure(frames.error());
        }
        OriginalSkill32Visual result;
        result.frames_ = std::move(frames).value();
        result.duration_ = 50;
        return Result<OriginalSkill32Visual>::success(std::move(result));
    }
    static Result<OriginalSkill32Visual> create_curse33(const std::filesystem::path& root)
    {
        auto loaded = read_spell_entry(root, 33, 33, 0, 0);
        if (!loaded) { return Result<OriginalSkill32Visual>::failure(loaded.error()); }
        const auto& entry = loaded.value();
        ResourceReader reader(entry);
        constexpr std::array<unsigned, 8> header{ 21, 0, 0, 71, 344, 7, 0, 5 };
        for (unsigned i = 0; i < 8; ++i)
        {
            if (reader.number(i * 2) != header[i]) { return Result<OriginalSkill32Visual>::failure(make_error("Invalid curse33 header")); }
        }
        if (!reader.extent(16, 14 + 21 * 8)) { return Result<OriginalSkill32Visual>::failure(reader.error()); }
        OriginalSkill32Visual out;
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
                return Result<OriginalSkill32Visual>::failure(make_error("Invalid curse33 directory"));
            }
            auto frame = try_decode({ entry.begin() + data + offset, entry.begin() + data + offset + length }, 71, 344);
            if (!frame) { return Result<OriginalSkill32Visual>::failure(frame.error()); }
            out.frames_.push_back(std::move(frame).value());
            previous = offset + length;
        }
        if (previous != entry.size() - data) { return Result<OriginalSkill32Visual>::failure(make_error("Invalid curse33 trailing bytes")); }
        return Result<OriginalSkill32Visual>::success(std::move(out));
    }
    bool curse() const { return curse_; }
    int amount(unsigned color) const { return curse_ ? amounts_[color] : -1; }
    explicit OriginalSkill32Visual(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    const std::vector<Frame>& frames() const { return frames_; }
    const Frame& current() const { return frames_.at(frame_); }
    size_t frame() const { return frame_; }
    bool done() const { return frame_ == frames_.size(); }
    unsigned duration_ms() const { return duration_; }
    void presented() { presented_ = true; }
    void tick(unsigned milliseconds)
    {
        if (done())
        {
            return;
        }
        elapsed_ += milliseconds;
        if (presented_ && elapsed_ >= duration_)
        {
            ++frame_;
            elapsed_ = 0;
            presented_ = false;
        }
    }
    static void tint(std::vector<uint8_t>& indexed, const OriginalPalette& palette)
    {
        std::array<uint8_t, 256> colors{ };
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
};
}    //namespace s2ne
