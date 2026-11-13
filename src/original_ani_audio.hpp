#pragma once
#include "resource_result.hpp"
#include <functional>
namespace s2ne
{
// 原45EFFB先截断为百分秒，再由473192等待；不可直接使用FLIC毫秒速度。
inline unsigned original_ani_frame_ms(unsigned type, unsigned speed)
{
    if (type == 0xAF12) { return (speed * 10 / 48) * 10; }
    if (type == 0xAF11) { return (speed * 4 / 3) * 10; }
    return 0;
}
// 45EC16／45F347：记录索引是解码后的ANI帧（从1开始），不是CEL帧。
class OriginalAniAudio
{
    struct Cue
    {
        unsigned frame;
        int music;
        int sound;
    };
    std::vector<std::vector<Cue>> scripts_;
    size_t script_{}, cursor_{};
    unsigned frame_{}, channel_{}, flags_{};
    int repeating_{};
    bool active_{};

public:
    using Music = std::function<Result<bool>(unsigned)>;
    Music play_music;
    Music play_midi;
    using Play = std::function<Result<unsigned>(unsigned, unsigned)>;
    using Stop = std::function<Result<bool>()>;
    static Result<OriginalAniAudio> create(const std::filesystem::path& root)
    {
        auto pal = read_resource(root / "SUPERSP.PAL");
        auto bin = read_resource(root / "SUPERSP.BIN");
        if (!pal) { return Result<OriginalAniAudio>::failure(pal.error()); }
        if (!bin) { return Result<OriginalAniAudio>::failure(bin.error()); }
        ResourceReader p(pal.value());
        const auto offset = p.number(896, 4), length = p.number(900, 4);
        if (!p || offset < 17 || length < 17) { return Result<OriginalAniAudio>::failure(make_error("Invalid ANI index")); }
        ResourceReader b(bin.value());
        if (!b.extent(offset - 17, length - 17)) { return Result<OriginalAniAudio>::failure(b.error()); }
        return parse({ bin.value().begin() + offset - 17, bin.value().begin() + offset + length - 34 });
    }
    static Result<OriginalAniAudio> parse(const std::vector<uint8_t>& bytes)
    {
        ResourceReader r(bytes);
        const unsigned count = r.number(0);
        if (!r.extent(2, size_t(count) * 21)) { return Result<OriginalAniAudio>::failure(r.error()); }
        OriginalAniAudio result;
        for (unsigned i = 0; i < count; ++i)
        {
            const auto offset = r.number(2 + i * 21, 4), n = r.number(21 + i * 21);
            if (!r.extent(offset, size_t(n) * 6)) { return Result<OriginalAniAudio>::failure(r.error()); }
            std::vector<Cue> cues;
            unsigned previous = 0;
            for (unsigned j = 0; j < n; ++j)
            {
                unsigned frame = r.number(offset + j * 6);
                int sound = int(int16_t(r.number(offset + j * 6 + 4)));
                if (!frame || frame < previous || sound < -245 || sound > 245)
                {
                    return Result<OriginalAniAudio>::failure(make_error("Invalid ANI audio cue"));
                }
                int music = int(int16_t(r.number(offset + j * 6 + 2)));
                if (music < -1 || music > 37) { return Result<OriginalAniAudio>::failure(make_error("Invalid ANI music cue")); }
                cues.push_back({ frame, music, sound });
                previous = frame;
            }
            result.scripts_.push_back(std::move(cues));
        }
        return Result<OriginalAniAudio>::success(std::move(result));
    }
    Result<bool> begin(unsigned script, unsigned flags)
    {
        if (!script || script > scripts_.size()) { return Result<bool>::failure(make_error("Invalid ANI script")); }
        if (!(flags & 1) && play_music)
        {
            auto stopped = play_music(0);
            if (!stopped) { return stopped; }
        }
        script_ = script - 1;
        flags_ = flags;
        cursor_ = frame_ = channel_ = 0;
        repeating_ = 0;
        active_ = true;
        return Result<bool>::success(true);
    }
    Result<bool> advance(unsigned frame, const Play& play, const Stop& stop)
    {
        if (!active_ || frame == frame_) { return Result<bool>::success(true); }
        if (frame != frame_ + 1) { return Result<bool>::failure(make_error("ANI audio frame discontinuity")); }
        frame_ = frame;
        const auto& cues = scripts_[script_];
        while (cursor_ < cues.size() && cues[cursor_].frame == frame)
        {
            const auto& cue = cues[cursor_++];
            if (play_music && cue.music != 0)
            {
                auto& callback = cue.music > 0 && play_midi ? play_midi : play_music;
                auto music = callback(cue.music == -1 ? 0u : unsigned(cue.music));
                if (!music) { return music; }
            }
            const int sound = cue.sound;
            if (sound == -1)
            {
                auto result = stop();
                if (!result) { return result; }
            }
            else if (sound > 0 || (sound < -1 && (flags_ & 0x400)))
            {
                unsigned requested = 0;
                unsigned id = unsigned(sound);
                if (sound < -1)
                {
                    id = unsigned(-sound);
                    requested = channel_;
                    repeating_ = int(id);
                }
                auto result = play(id, requested);
                if (!result) { return Result<bool>::failure(result.error()); }
                if (sound < -1) { channel_ = result.value(); }
            }
        }
        return poll(play);
    }
    Result<bool> poll(const Play& play)
    {
        if (active_ && (flags_ & 0x400) && repeating_ > 0 && channel_)
        {
            auto result = play(unsigned(repeating_), channel_);
            if (!result) { return Result<bool>::failure(result.error()); }
            channel_ = result.value();
        }
        return Result<bool>::success(true);
    }
    Result<bool> finish(const Stop& stop)
    {
        if (!active_) { return Result<bool>::success(true); }
        active_ = false;
        if (!(flags_ & 1) && play_music)
        {
            auto music = play_music(0);
            if (!music) { return music; }
        }
        if (!(flags_ & 2)) { return stop(); }
        return Result<bool>::success(true);
    }
};
}    //namespace s2ne
