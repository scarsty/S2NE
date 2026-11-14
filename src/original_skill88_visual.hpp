#pragma once
#include "native_ability_buffs.hpp"
#include "native_damage_records.hpp"
#include "native_timed_buffs.hpp"
#include "original_skill_visual.hpp"
namespace s2ne
{
class OriginalSkill88Visual
{
    std::vector<OriginalSkill32Visual::Frame> frames_;
    std::array<int, 256> amounts_{};
    size_t frame_{};
    uint64_t elapsed_{};
    int anchor_x_{ 89 }, anchor_y_{ 124 };
    unsigned tint_amount_{ 50 }, tint_source_{ 233 };
    bool presented_{};
    unsigned frame_delay_{ 50 };
    unsigned prefix_frames_{};
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }

public:
    static std::vector<uint8_t> shake_frame(const std::vector<uint8_t>& source, const std::vector<uint8_t>& previous, unsigned remaining)
    {
        if (source.size() != 640u * 480 * 4 || previous.size() != source.size() || !remaining)
        {
            fatal("46C34D invalid shake framebuffer");
        }
        auto image = previous;
        if (remaining & 1) { std::copy_n(source.begin() + 8, 307198 * 4, image.begin()); }
        else
        {
            std::copy_n(source.begin(), 307198 * 4, image.begin() + 8);
        }
        return image;
    }
    OriginalSkill88Visual() = default;
    static Result<OriginalSkill88Visual> create(const std::filesystem::path& root, bool healing1 = false, bool blessing3 = false)
    {
        unsigned skill = 88, visual = 78, tint = 50;
        if (blessing3)
        {
            skill = visual = 3;
            tint = 40;
        }
        else if (healing1)
        {
            skill = visual = 1;
            tint = 0;
        }
        auto loaded = read_spell_entry(root, skill, visual, 233, tint);
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
        std::array<unsigned int, 8> header;
        if (blessing3) { header = std::array<unsigned, 8>{ 27, 81, 390, 161, 401, 6, 0, 5 }; }
        else if (healing1) { header = std::array<unsigned, 8>{ 26, 0, 0, 132, 148, 0, 0, 5 }; }
        else
        {
            header = std::array<unsigned, 8>{ 35, 89, 124, 175, 124, 6, 0, 5 };
        }

        result.anchor_x_ = 89;
        result.anchor_y_ = 124;
        if (blessing3)
        {
            result.anchor_x_ = 81;
            result.anchor_y_ = 390;
        }
        else if (healing1)
        {
            result.anchor_x_ = 66;
            result.anchor_y_ = 148;
        }
        result.tint_amount_ = tint;
        for (unsigned i = 0; i < 8; ++i)
        {
            if (word(entry, i * 2) != header[i])
            {
                return Result<OriginalSkill88Visual>::failure(make_error("Unsupported88 SPELLSP layout"));
            }
        }
        // 463EA6 maps six source indices to their independent blend percentages.
        for (unsigned i = 0; i < header[5]; ++i)
        {
            result.amounts_[entry.at(16 + i)] = entry.at(16 + header[5] + i);
        }
        const size_t directory = 16 + header[5] * 2;
        size_t data = directory + header[0] * 8, previous = 0;
        if (entry.size() < data)
        {
            return Result<OriginalSkill88Visual>::failure(make_error("Truncated88 directory"));
        }
        for (unsigned i = 0; i < header[0]; ++i)
        {
            size_t off = dword(entry, directory + i * 8), size = dword(entry, directory + 4 + i * 8);
            if (off != previous || off > entry.size() - data || size > entry.size() - data - off)
            {
                return Result<OriginalSkill88Visual>::failure(make_error("Invalid88 frame extent"));
            }
            auto frame = OriginalSkill32Visual::try_decode({ entry.begin() + data + off, entry.begin() + data + off + size }, header[3], header[4]);
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
    static Result<OriginalSkill88Visual> create_sleep2(const std::filesystem::path& root)
    {
        auto loaded = read_spell_entry(root, 2, 2, 59, 25);
        if (!loaded) { return Result<OriginalSkill88Visual>::failure(loaded.error()); }
        const auto& entry = loaded.value();
        ResourceReader reader(entry);
        constexpr std::array<unsigned, 8> header{ 16, 84, 100, 167, 167, 4, 40, 5 };
        for (unsigned i = 0; i < 8; ++i)
        {
            if (reader.number(i * 2) != header[i]) { return Result<OriginalSkill88Visual>::failure(make_error("Sleep2 header")); }
        }
        if (!reader.extent(16, 48 + 16 * 8)) { return Result<OriginalSkill88Visual>::failure(reader.error()); }
        OriginalSkill88Visual result;
        result.anchor_x_ = 84;
        result.anchor_y_ = 100;
        result.tint_source_ = 59;
        result.tint_amount_ = 25;
        result.amounts_.fill(-1);
        for (unsigned i = 0; i < 4; ++i) { result.amounts_[entry[16 + i]] = entry[20 + i]; }
        std::vector<OriginalSkill32Visual::Frame> frames;
        constexpr size_t directory = 64, data = 192;
        size_t previous = 0;
        for (unsigned i = 0; i < 16; ++i)
        {
            size_t off = reader.number(directory + i * 8, 4), size = reader.number(directory + i * 8 + 4, 4);
            if (!reader || off != previous || off > entry.size() - data || size > entry.size() - data - off) { return Result<OriginalSkill88Visual>::failure(make_error("Sleep2 frame directory")); }
            auto decoded = OriginalSkill32Visual::try_decode({ entry.begin() + data + off, entry.begin() + data + off + size }, 167, 167);
            if (!decoded) { return Result<OriginalSkill88Visual>::failure(decoded.error()); }
            frames.push_back(std::move(decoded).value());
            previous = off + size;
        }
        if (previous != entry.size() - data) { return Result<OriginalSkill88Visual>::failure(make_error("Sleep2 trailing bytes")); }
        for (unsigned i = 0; i < 40; ++i)
        {
            unsigned index = entry[24 + i];
            if (!index || index > frames.size()) { return Result<OriginalSkill88Visual>::failure(make_error("Sleep2 frame order")); }
            result.frames_.push_back(frames[index - 1]);
        }
        return Result<OriginalSkill88Visual>::success(std::move(result));
    }
    static Result<OriginalSkill88Visual> create_strengthening_status(const std::filesystem::path& root, unsigned skill, unsigned cursor = 0)
    {
        const auto* buff = NativeTimedBuffs::find(skill);
        const auto* damage = NativeDamageRecords::find(skill);
        const bool summon = skill >= 64 && skill <= 74;
        if (cursor && ((skill != 108 && skill != 51) || cursor != 1)) { return Result<OriginalSkill88Visual>::failure(make_error("Unsupported spell resource cursor")); }
        if (!summon && !damage && !buff && !NativeAbilityBuffs::find(skill) && skill != 51 && skill != 29 && skill != 18 && skill != 40 && skill != 45 && skill != 50 && skill != 8 && skill != 12 && skill != 35 && skill != 39 && skill != 48 && skill != 56 && skill != 60 && skill != 17 && skill != 25 && skill != 52 && skill != 61 && skill != 4 && skill != 5 && skill != 6 && skill != 9 && skill != 10 && skill != 13 && skill != 15 && skill != 53 && skill != 34 && skill != 38 && skill != 54 && skill != 96) { return Result<OriginalSkill88Visual>::failure(make_error("Unsupported support visual ID")); }
        const auto* ability = NativeAbilityBuffs::find(skill);
        unsigned color;

        if (damage) { color = damage->color; }
        else if (summon) { color = 233; }
        else if (skill == 51) { color = 0; }
        else if (skill == 29) { color = 106; }
        else if (skill == 18 || skill == 40) { color = 233; }
        else if (skill == 45) { color = 59; }
        else if (skill == 50) { color = 233; }
        else if (ability) { color = ability->color; }
        else if (buff) { color = buff->color; }
        else if (skill == 8) { color = 176; }
        else if (skill == 35) { color = 139; }
        else if (skill == 48) { color = 245; }
        else if (skill == 60) { color = 107; }
        else if (skill == 12 || skill == 39 || skill == 56) { color = 233; }
        else if (skill == 25) { color = 223; }
        else if (skill == 17 || skill == 52 || skill == 61) { color = 233; }
        else if (skill == 54) { color = 177; }
        else if (skill <= 5 || skill == 13 || skill == 15 || skill == 53 || skill == 34 || skill == 38 || skill == 96) { color = 233; }
        else
        {
            color = 176;
        }

        unsigned tint;

        if (damage) { tint = damage->tint; }
        else if (summon) { tint = 50; }
        else if (skill == 51) { tint = 0; }
        else if (skill == 29) { tint = 45; }
        else if (skill == 18) { tint = 15; }
        else if (skill == 40) { tint = 45; }
        else if (skill == 45) { tint = 25; }
        else if (skill == 50) { tint = 75; }
        else if (ability) { tint = ability->tint; }
        else if (buff) { tint = buff->tint; }
        else if (skill == 8 || skill == 39) { tint = 50; }
        else if (skill == 35) { tint = 0; }
        else if (skill == 48) { tint = 65; }
        else if (skill == 56 || skill == 60) { tint = 35; }
        else if (skill == 17) { tint = 0; }
        else if (skill == 25) { tint = 35; }
        else if (skill == 52) { tint = 65; }
        else if (skill == 61) { tint = 60; }
        else if (skill == 34 || skill == 54 || skill == 96) { tint = 35; }
        else if (skill == 38) { tint = 60; }
        else if (skill == 53) { tint = 50; }
        else if (skill == 15) { tint = 60; }
        else if (skill == 4) { tint = 15; }
        else if (skill == 5 || skill == 10) { tint = 60; }
        else if (skill == 6) { tint = 75; }
        else
        {
            tint = 25;
        }

        unsigned visual = skill;
        if (damage) { visual = damage->visual; }
        else if (summon) { visual = 65; }
        else if (skill == 51) { visual = 52; }
        else if (skill == 50) { visual = 51; }
        else if (ability) { visual = ability->visual; }
        else if (buff) { visual = buff->visual; }
        else
        {
            switch (skill)
            {
            case 48: visual = 49; break;
            case 56: visual = 58; break;
            case 60: visual = 62; break;
            case 52: visual = 54; break;
            case 61: visual = 63; break;
            case 96: visual = 89; break;
            case 53: visual = 55; break;
            case 54: visual = 56; break;
            default: break;
            }
        }
        unsigned dispatch = 0;
        if (damage) { dispatch = damage->dispatch; }
        if (skill == 78) { dispatch = 9; }
        if (skill == 47) { dispatch = 1; }
        if (skill == 29) { dispatch = 7; }
        if (skill == 51) { dispatch = 5; }
        auto loaded = read_spell_entry(root, skill, visual, color, tint, dispatch, cursor);
        if (!loaded) { return Result<OriginalSkill88Visual>::failure(loaded.error()); }
        const auto& entry = loaded.value();
        ResourceReader reader(entry);
        std::array<unsigned, 8> header{};
        if (damage) { header = damage->header; }
        else if (summon) { header = std::array<unsigned, 8>{ 46, 0, 0, 192, 290, 0, 55, 4 }; }
        else if (skill == 51 && !cursor) { header = std::array<unsigned, 8>{ 7, 0, 0, 70, 123, 0, 0, 3 }; }
        else if (skill == 51) { header = std::array<unsigned, 8>{ 6, 0, 0, 97, 391, 0, 0, 3 }; }
        else if (skill == 29) { header = std::array<unsigned, 8>{ 14, 56, 31, 111, 65, 1, 22, 4 }; }
        else if (skill == 18) { header = std::array<unsigned, 8>{ 22, 86, 345, 172, 361, 7, 27, 4 }; }
        else if (skill == 40) { header = std::array<unsigned, 8>{ 14, 0, 0, 109, 344, 0, 26, 5 }; }
        else if (skill == 45) { header = std::array<unsigned, 8>{ 14, 0, 0, 63, 241, 4, 22, 5 }; }
        else if (skill == 50) { header = std::array<unsigned, 8>{ 1, 65535, 65535, 640, 480, 0, 65516, 5 }; }
        else if (ability) { header = ability->header; }
        else if (buff) { header = buff->header; }
        else
        {
            if (skill == 8) { header = std::array<unsigned, 8>{ 37, 37, 351, 80, 366, 8, 0, 5 }; }
            else if (skill == 12) { header = std::array<unsigned, 8>{ 32, 0, 0, 51, 104, 0, 0, 5 }; }
            else if (skill == 35) { header = std::array<unsigned, 8>{ 14, 0, 0, 89, 170, 0, 22, 5 }; }
            else if (skill == 39) { header = std::array<unsigned, 8>{ 30, 145, 158, 287, 206, 0, 0, 5 }; }
            else if (skill == 48) { header = std::array<unsigned, 8>{ 34, 65535, 65535, 640, 480, 1, 36, 4 }; }
            else if (skill == 56) { header = std::array<unsigned, 8>{ 32, 0, 0, 137, 138, 0, 34, 5 }; }
            else if (skill == 60) { header = std::array<unsigned, 8>{ 20, 0, 0, 100, 140, 0, 33, 5 }; }
            else if (skill == 17) { header = std::array<unsigned, 8>{ 31, 0, 0, 139, 221, 17, 0, 5 }; }
            else if (skill == 25) { header = std::array<unsigned, 8>{ 12, 0, 0, 126, 158, 0, 28, 5 }; }
            else if (skill == 52) { header = std::array<unsigned, 8>{ 63, 119, 265, 237, 463, 1, 0, 5 }; }
            else if (skill == 61) { header = std::array<unsigned, 8>{ 32, 65535, 65535, 640, 480, 0, 0, 5 }; }
            else if (skill == 96) { header = std::array<unsigned, 8>{ 30, 0, 0, 111, 249, 0, 0, 5 }; }
            else if (skill == 34) { header = std::array<unsigned, 8>{ 27, 89, 70, 177, 99, 0, 36, 5 }; }
            else if (skill == 38) { header = std::array<unsigned, 8>{ 30, 60, 250, 120, 289, 0, 0, 5 }; }
            else if (skill == 54) { header = std::array<unsigned, 8>{ 26, 0, 0, 125, 125, 0, 0, 4 }; }
            else if (skill == 13) { header = std::array<unsigned, 8>{ 25, 0, 0, 69, 354, 9, 0, 4 }; }
            else if (skill == 15) { header = std::array<unsigned, 8>{ 38, 80, 261, 165, 384, 5, 0, 5 }; }
            else if (skill == 53) { header = std::array<unsigned, 8>{ 27, 65535, 65535, 640, 480, 0, 0, 4 }; }
            else if (skill == 4) { header = std::array<unsigned, 8>{ 24, 0, 0, 97, 242, 4, 0, 5 }; }
            else if (skill == 5) { header = std::array<unsigned, 8>{ 11, 0, 0, 128, 213, 0, 22, 4 }; }
            else if (skill == 6) { header = std::array<unsigned, 8>{ 9, 65535, 65535, 640, 480, 0, 38, 4 }; }
            else if (skill == 9) { header = std::array<unsigned, 8>{ 33, 84, 208, 165, 263, 7, 0, 5 }; }
            else
            {
                header = std::array<unsigned, 8>{ 59, 65535, 65535, 640, 480, 1, 0, 5 };
            }
        }

        if (cursor && skill != 51) { header = { 16, 155, 280, 312, 398, 0, 0, 4 }; }
        for (unsigned i = 0; i < 8; ++i)
        {
            if (reader.number(i * 2) != header[i]) { return Result<OriginalSkill88Visual>::failure(make_error("004/005 CEL header")); }
        }
        OriginalSkill88Visual result;
        result.anchor_x_ = int(header[3] / 2);
        if (header[1] == 65535) { result.anchor_x_ = -1; }
        else if (header[1]) { result.anchor_x_ = int(header[1]); }
        result.anchor_y_ = int(header[4]);
        if (header[2] == 65535) { result.anchor_y_ = -1; }
        else if (header[2]) { result.anchor_y_ = int(header[2]); }
        if (dispatch == 6 || dispatch == 10)
        {
            // 46BD98/46D072 use the cell-local bitmap size, not the ordinary CEL anchor.
            result.anchor_x_ = int(header[3] / 2);
            result.anchor_y_ = int(header[4]);
        }
        result.tint_amount_ = tint;
        result.tint_source_ = color;
        result.frame_delay_ = header[7] * 10;
        result.amounts_.fill(-1);
        // 463EA6 advances past an order table only for a positive signed count.
        const int order_count = int16_t(header[6]);
        size_t order_bytes = 0;
        if (order_count > 0) { order_bytes = unsigned(order_count); }
        const size_t directory = 16 + header[5] * 2 + order_bytes, data = directory + header[0] * 8;
        if (!reader.extent(16, data - 16)) { return Result<OriginalSkill88Visual>::failure(reader.error()); }
        for (unsigned i = 0; i < header[5]; ++i) { result.amounts_[entry[16 + i]] = entry[16 + header[5] + i]; }
        std::vector<OriginalSkill32Visual::Frame> frames;
        size_t previous = 0;
        for (unsigned i = 0; i < header[0]; ++i)
        {
            size_t off = reader.number(directory + i * 8, 4), size = reader.number(directory + i * 8 + 4, 4);
            if (!reader || off != previous || off > entry.size() - data || size > entry.size() - data - off) { return Result<OriginalSkill88Visual>::failure(make_error("004/005 CEL extent")); }
            auto frame = OriginalSkill32Visual::try_decode({ entry.begin() + data + off, entry.begin() + data + off + size }, header[3], header[4]);
            if (!frame) { return Result<OriginalSkill88Visual>::failure(frame.error()); }
            frames.push_back(std::move(frame).value());
            previous = off + size;
        }
        if (previous != entry.size() - data) { return Result<OriginalSkill88Visual>::failure(make_error("004/005 CEL trailing bytes")); }
        if (order_count <= 0)
        {
            // 46B9A9/46BD98 repeat the whole directory abs(count)+1 times.
            const unsigned passes = unsigned(-order_count) + 1;
            for (unsigned pass = 0; pass < passes; ++pass)
            {
                for (const auto& frame : frames) { result.frames_.push_back(frame); }
            }
        }
        else
        {
            for (unsigned i = 0; i < order_bytes; ++i)
            {
                unsigned index = entry[16 + header[5] * 2 + i];
                if (!index || index > frames.size()) { return Result<OriginalSkill88Visual>::failure(make_error("005 CEL order")); }
                result.frames_.push_back(frames[index - 1]);
            }
        }
        return Result<OriginalSkill88Visual>::success(std::move(result));
    }
    unsigned tint_source() const { return tint_source_; }
    explicit OriginalSkill88Visual(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    int anchor_x() const { return anchor_x_; }
    int anchor_y() const { return anchor_y_; }
    unsigned tint_amount() const { return tint_amount_; }
    const auto& frames() const { return frames_; }
    const auto& current() const { return frames_.at(frame_); }
    const auto& frame_at(size_t index) const { return frames_.at(index); }
    unsigned frame_delay() const { return frame_delay_; }
    void prepend_timeline(unsigned count)
    {
        const auto first = frames_.front();
        frames_.insert(frames_.begin(), count, first);
        prefix_frames_ = count;
    }
    void extend_timeline(unsigned count)
    {
        const auto last = frames_.back();
        for (unsigned i = 0; i < count; ++i) { frames_.push_back(last); }
    }
    int amount(unsigned color) const { return amounts_.at(color); }
    size_t frame() const { return frame_; }
    bool done() const { return frame_ == frames_.size(); }
    void presented() { presented_ = true; }
    void tick(unsigned ms, bool accelerated = false)
    {
        if (done())
        {
            return;
        }
        elapsed_ += ms;
        unsigned delay = frame_ < prefix_frames_ ? 20u : frame_delay_;
        if (accelerated) { delay = std::max(1u, delay / 4); }
        if (presented_ && elapsed_ >= delay)
        {
            if (accelerated)
            {
                ++frame_;
                elapsed_ = 0;
            }
            else
            {
                ++frame_;
                elapsed_ = 0;
            }
            presented_ = false;
        }
    }
};
}    //namespace s2ne
