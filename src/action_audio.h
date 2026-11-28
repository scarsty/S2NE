#pragma once
#include "resource_result.h"
#include <algorithm>
namespace s2ne
{
// 原428658：四组115字节分发表之后，长度前缀指向505C28声音表。
Result<std::vector<uint8_t>> original_skill_sounds(const std::filesystem::path& root, unsigned cursor = 0);
// 时间线只消费已启动阶段；取消丢弃未到达的cue，不臆加原函数没有的stop。
class ActionAudioCues
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
    void begin(std::vector<Cue> cues);
    void cancel();
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
unsigned original_movement_sound(unsigned mode, int appearanceType, int ground, unsigned terrain);
// 原43E1FC：动作5固定127，其余使用record+0Dh的有符号WORD，不是命中声+0Fh。
inline int original_action_pose_sound(const auto& fields)
{
    if (fields[0] == 5) { return 127; }
    return int(int16_t(uint16_t(uint8_t(fields[13])) | uint16_t(uint8_t(fields[14])) << 8));
}
}    //namespace s2ne
