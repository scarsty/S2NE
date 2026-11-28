#pragma once
#include "resource_result.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <array>
namespace s2ne
{
int original_effect_channel(unsigned id, unsigned requested, unsigned lastId, int last,
    std::span<const bool> loaded, std::span<const bool> playing);
class OriginalEffectAudio
{
    MIX_Mixer* mixer_{};
    std::array<MIX_Track*, 16> tracks_{};
    std::array<MIX_Audio*, 245> decoded_{};
    std::array<unsigned, 16> loaded_{};
    std::vector<uint8_t> sounds_;
    unsigned last_id_{};
    int last_ = -1, volume_ = 100;
    static unsigned number(std::span<const uint8_t> b, size_t p, unsigned n = 4);

public:
    OriginalEffectAudio() = default;
    OriginalEffectAudio(const OriginalEffectAudio&) = delete;
    OriginalEffectAudio& operator=(const OriginalEffectAudio&) = delete;
    ~OriginalEffectAudio();
    Result<bool> start(MIX_Mixer* mixer, const std::filesystem::path& root);
    Result<bool> volume(int value);
    Result<unsigned> play(unsigned id, unsigned channel = 0);
    Result<bool> stop();
};
}    //namespace s2ne
