#pragma once
#include "resource_result.hpp"
#include <algorithm>
namespace s2ne
{
// 原428658：四组115字节分发表之后，长度前缀指向505C28声音表。
inline Result<std::vector<uint8_t>> original_skill_sounds(const std::filesystem::path& root, unsigned cursor = 0)
{
    auto pal = read_resource(root / "SUPERSP.PAL");
    auto bin = read_shared_resource(root / "SUPERSP.BIN");
    if (!pal) { return Result<std::vector<uint8_t>>::failure(pal.error()); }
    if (!bin) { return Result<std::vector<uint8_t>>::failure(bin.error()); }
    ResourceReader p(pal.value()), b(bin.value());
    unsigned encoded = p.number(920, 4);
    if (!p || encoded < 20) { return Result<std::vector<uint8_t>>::failure(make_error("Invalid skill sound table")); }
    size_t start = encoded - 20;
    unsigned count = b.number(start);
    size_t sounds = start + 2 + size_t(count) * 4;
    unsigned length = b.number(sounds);
    if (count != 115 || length < count || !b.extent(sounds + 2, length))
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid skill sound extent"));
    }
    std::vector<uint8_t> result(count);
    for (unsigned i = 0; i < count; ++i)
    {
        const unsigned visual = b.number(start + 2 + i, 1) + cursor;
        if (visual >= length) { return Result<std::vector<uint8_t>>::failure(make_error("Skill sound visual index")); }
        result[i] = uint8_t(b.number(sounds + 2 + visual, 1));
    }
    return Result<std::vector<uint8_t>>::success(std::move(result));
}
// 时间线只消费已启动阶段；取消丢弃未到达的cue，不臆加原函数没有的stop。
class OriginalActionAudioCues
{
public:
    struct Cue
    {
        size_t frame;
        unsigned sound;
    };

private:
    std::vector<Cue> cues_;
    size_t next_{};

public:
    void begin(std::vector<Cue> cues)
    {
        std::stable_sort(cues.begin(), cues.end(), [](const Cue& a, const Cue& b)
            {
                return a.frame < b.frame;
            });
        cues_ = std::move(cues);
        next_ = 0;
    }
    void cancel()
    {
        cues_.clear();
        next_ = 0;
    }
    template <class Play>
    Result<bool> advance(size_t frame, const Play& play)
    {
        while (next_ < cues_.size() && cues_[next_].frame <= frame)
        {
            const auto& cue = cues_[next_];
            if (cue.sound)
            {
                auto result = play(cue.sound, 0);
                if (!result) { return Result<bool>::failure(result.error()); }
            }
            ++next_;
        }
        return Result<bool>::success(true);
    }
};
template <class Play, class Stop>
Result<bool> original_script_sound(int sound, const Play& play, const Stop& stop)
{
    if (!sound) { return stop(); }
    if (sound < 0 || sound > 245) { return Result<bool>::failure(make_error("Invalid script sound")); }
    auto result = play(unsigned(sound), 0);
    if (!result) { return Result<bool>::failure(result.error()); }
    return Result<bool>::success(true);
}
// 452167: movement mode is 440F56; appearance byte18 is a separate test.
inline unsigned original_movement_sound(unsigned mode, int appearanceType, int ground, unsigned terrain)
{
    if (mode == 4 || mode == 5) { return 132; }
    if (mode == 3) { return 133; }
    if (mode == 1) { return 122; }
    if (ground > 0 && ground <= 4 && (terrain == 1 || terrain == 2)) { return 131; }
    if (appearanceType == 4) { return 124; }
    return 103;
}
// 原43E1FC：动作5固定127，其余使用record+0Dh的有符号WORD，不是命中声+0Fh。
inline int original_action_pose_sound(const auto& fields)
{
    if (fields[0] == 5) { return 127; }
    return int(int16_t(uint16_t(uint8_t(fields[13])) | uint16_t(uint8_t(fields[14])) << 8));
}
}    //namespace s2ne
