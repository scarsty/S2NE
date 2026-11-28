#pragma once
#include "effect_audio.h"
#include "music.h"
#include "runtime_error.h"
#include "xmi.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <fluidsynth.h>
#include <iostream>
#include <memory>

namespace s2ne
{
class OpeningAudio
{
    MIX_Mixer* mixer{};
    MIX_Audio* audio{};
    MIX_Track* track{};
    bool initialized{};
    bool muted{};
    bool diagnostics_ = true;
    int music_volume_{ 100 };
    std::filesystem::path music_path_;
    unsigned music_id_{};
    bool music_midi_{};
    std::vector<std::vector<uint8_t>> xmi_resources_;
    int midi_volume_{ 100 }, cd_volume_{ 100 };
    std::unique_ptr<EffectAudio> effects_;
    std::atomic<unsigned long long> signal_samples{ 0 };
    static void fluid_log(int level, const char* message, void*);
    static void SDLCALL monitor(void* userdata, MIX_Mixer*, const SDL_AudioSpec*, float* pcm, int samples);

public:
    unsigned music_id() const;
    unsigned long long mixed_samples() const;
    Sint64 music_position() const;
    Result<bool> music_volume(int value);
    Result<bool> stop_music();
    Result<bool> music_volumes(int midi, int cd);
    bool music_is_midi() const;
    Result<bool> play_music(const std::filesystem::path& root, unsigned id, ResourceDomain domain, bool force_midi = false);
    MIX_Mixer* mixer_handle() const;
    Result<bool> start_effects(const std::filesystem::path& root);
    EffectAudio* effects() const;
    OpeningAudio();
    OpeningAudio(const OpeningAudio&) = delete;
    void set_diagnostics(bool enabled);
    OpeningAudio& operator=(const OpeningAudio&) = delete;
    ~OpeningAudio();
    void stop();
    Result<bool> start(const std::filesystem::path& path);
    Result<bool> replay();
    Result<bool> toggle();
    bool is_muted() const;
    bool progressing() const;
};
}    //namespace s2ne

#include "terminal_output.h"