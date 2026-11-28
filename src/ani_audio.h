#pragma once
#include "resource_result.h"
#include <functional>
namespace s2ne
{
// 原45EFFB先截断为百分秒，再由473192等待；不可直接使用FLIC毫秒速度。
unsigned original_ani_frame_ms(unsigned type, unsigned speed);
// 45EC16／45F347：记录索引是解码后的ANI帧（从1开始），不是CEL帧。
class AniAudio
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
    static Result<AniAudio> create(const std::filesystem::path& root);
    static Result<AniAudio> parse(const std::vector<uint8_t>& bytes);
    Result<bool> begin(unsigned script, unsigned flags);
    Result<bool> advance(unsigned frame, const Play& play, const Stop& stop);
    Result<bool> poll(const Play& play);
    Result<bool> finish(const Stop& stop);
};
}    //namespace s2ne
