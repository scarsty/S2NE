#pragma once
#include <SDL3_mixer/SDL_mixer.h>
#include <filesystem>
#include <stdexcept>
#include <atomic>
#include <cmath>
#include <iostream>

namespace s2ne {
class OpeningAudio {
    MIX_Mixer* mixer{};
    MIX_Audio* audio{};
    MIX_Track* track{};
    bool initialized{};
    bool muted{};
    std::atomic<unsigned long long> signal_samples{0};
    static void SDLCALL monitor(void* userdata, MIX_Mixer*, const SDL_AudioSpec*, float* pcm, int samples) {
        auto* self = static_cast<OpeningAudio*>(userdata);
        unsigned long long audible = 0;
        for (int i = 0; i < samples; ++i) if (std::abs(pcm[i]) > 0.0001f) ++audible;
        self->signal_samples.fetch_add(audible, std::memory_order_relaxed);
    }
public:
    OpeningAudio() = default;
    OpeningAudio(const OpeningAudio&) = delete;
    OpeningAudio& operator=(const OpeningAudio&) = delete;
    ~OpeningAudio() { stop(); }
    void stop() {
        MIX_DestroyTrack(track); track = nullptr;
        MIX_DestroyAudio(audio); audio = nullptr;
        MIX_DestroyMixer(mixer); mixer = nullptr;
        if (initialized) { MIX_Quit(); initialized = false; }
    }
    void start(const std::filesystem::path& path) {
        if (!MIX_Init()) throw std::runtime_error(SDL_GetError());
        initialized = true;
        SDL_AudioDeviceID chosen = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
        const char* requested = SDL_getenv("S2NE_AUDIO_DEVICE");
        if (requested && *requested) {
            int count = 0;
            auto* devices = SDL_GetAudioPlaybackDevices(&count);
            for (int i = 0; devices && i < count; ++i) {
                const char* name = SDL_GetAudioDeviceName(devices[i]);
                if (name && std::string(name).find(requested) != std::string::npos) { chosen = devices[i]; break; }
            }
            SDL_free(devices);
            if (chosen == SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK) throw std::runtime_error("Requested S2NE_AUDIO_DEVICE was not found");
        }
        mixer = MIX_CreateMixerDevice(chosen, nullptr);
        if (!mixer) throw std::runtime_error(SDL_GetError());
        if (!MIX_SetMixerGain(mixer, 1.f) || !MIX_SetPostMixCallback(mixer, monitor, this)) throw std::runtime_error(SDL_GetError());
        std::cout << "Audio driver: " << SDL_GetCurrentAudioDriver() << '\n';
        int device_count = 0;
        auto* devices = SDL_GetAudioPlaybackDevices(&device_count);
        for (int i = 0; devices && i < device_count; ++i) std::cout << "Playback device: " << SDL_GetAudioDeviceName(devices[i]) << '\n';
        SDL_free(devices);
        const auto utf8 = path.u8string();
        audio = MIX_LoadAudio(mixer, reinterpret_cast<const char*>(utf8.c_str()), true);
        if (!audio || MIX_GetAudioDuration(audio) <= 0) throw std::runtime_error(SDL_GetError());
        track = MIX_CreateTrack(mixer);
        if (!track || !MIX_SetTrackAudio(track, audio) || !MIX_SetTrackGain(track, 1.f)) throw std::runtime_error(SDL_GetError());
        replay();
    }
    void replay() {
        auto props = SDL_CreateProperties();
        if (!props) throw std::runtime_error(SDL_GetError());
        SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
        bool ok = MIX_PlayTrack(track, props);
        SDL_DestroyProperties(props);
        if (!ok) throw std::runtime_error(SDL_GetError());
    }
    void toggle() {
        muted = !muted;
        if (!MIX_SetTrackGain(track, muted ? 0.f : 1.f)) throw std::runtime_error(SDL_GetError());
    }
    bool is_muted() const { return muted; }
    bool progressing() const {
        std::cout << "Non-silent mixed samples: " << signal_samples.load() << '\n';
        return MIX_TrackPlaying(track) && MIX_GetTrackPlaybackPosition(track) > 0 && signal_samples.load() > 0;
    }
};
}
