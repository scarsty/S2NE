#include "opening_audio.h"

namespace s2ne
{
auto OpeningAudio::fluid_log(int level, const char* message, void*) -> void
{
    if (level <= FLUID_ERR) { std::println(stderr, "fluidsynth: {}", message ? message : ""); }
}

auto OpeningAudio::monitor(void* userdata, MIX_Mixer*, const SDL_AudioSpec*, float* pcm, int samples) -> void SDLCALL
{
    auto* self = static_cast<OpeningAudio*>(userdata);
    unsigned long long audible = 0;
    for (int i = 0; i < samples; ++i)
    {
        if (std::abs(pcm[i]) > 0.0001f)
        {
            ++audible;
        }
    }
    const auto previous = self->signal_samples.fetch_add(audible, std::memory_order_relaxed);
#ifdef __ANDROID__
    if (audible && previous == 0) { SDL_Log("Audio mixer produced non-silent PCM"); }
#else
    (void)previous;
#endif
}

auto OpeningAudio::music_id() const -> unsigned { return music_id_; }

auto OpeningAudio::mixed_samples() const -> unsigned long long { return signal_samples.load(); }

auto OpeningAudio::music_position() const -> Sint64 { return track ? MIX_GetTrackPlaybackPosition(track) : 0; }

auto OpeningAudio::music_volume(int value) -> Result<bool>
{
    value = std::clamp(value, 0, 100);
    if (track && !MIX_SetTrackGain(track, muted ? 0.f : value / 100.f))
    {
        return Result<bool>::failure(make_error(SDL_GetError()));
    }
    music_volume_ = value;
    return Result<bool>::success(true);
}

auto OpeningAudio::stop_music() -> Result<bool>
{
    if (track && !MIX_StopTrack(track, 0)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    music_id_ = 0;
    return Result<bool>::success(true);
}

auto OpeningAudio::music_volumes(int midi, int cd) -> Result<bool>
{
    midi_volume_ = midi;
    cd_volume_ = cd;
    return music_volume(music_midi_ ? midi : cd);
}

auto OpeningAudio::music_is_midi() const -> bool { return music_midi_; }

auto OpeningAudio::play_music(const std::filesystem::path& root, unsigned id, ResourceDomain domain, bool force_midi) -> Result<bool>
{
    if (!id) { return stop_music(); }
    if (id > 37) { return Result<bool>::failure(make_error("Invalid original music id")); }
    auto path = original_music_path(root, id, domain);
    bool midi = force_midi || !path;
    if (!mixer || !track) { return Result<bool>::failure(make_error("Music device unavailable")); }
    std::filesystem::path identity;
    if (midi) { identity = "midisp-" + std::to_string(id); }
    else
    {
        identity = path.value();
    }
    if (music_id_ == id && music_path_ == identity && MIX_TrackPlaying(track)) { return Result<bool>::success(true); }
    MIX_Audio* candidate = nullptr;
    if (midi)
    {
        if (xmi_resources_.empty())
        {
            auto resources = original_xmi_resources(root);
            if (!resources) { return Result<bool>::failure(resources.error()); }
            xmi_resources_ = std::move(resources).value();
        }
        auto converted = original_xmi_midi(xmi_resources_[id - 1]);
        if (!converted) { return Result<bool>::failure(converted.error()); }
        auto bank = root / "audio" / "GeneralUser-GS.sf2";
        if (!std::filesystem::is_regular_file(bank)) { bank = root / "S2NE" / "assets" / "audio" / "GeneralUser-GS.sf2"; }
        const char* override_bank = SDL_getenv("S2NE_SOUNDFONT");
        if (override_bank && *override_bank) { bank = std::filesystem::u8path(override_bank); }
        if (!std::filesystem::is_regular_file(bank)) { return Result<bool>::failure(make_error("GM SoundFont missing: " + bank.string())); }
        auto props = SDL_CreateProperties();
        auto* io = SDL_IOFromConstMem(converted.value().data(), converted.value().size());
        if (!props || !io)
        {
            if (props) { SDL_DestroyProperties(props); }
            if (io) { SDL_CloseIO(io); }
            return Result<bool>::failure(make_error(SDL_GetError()));
        }
        auto utf8 = bank.u8string();
        SDL_SetPointerProperty(props, MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io);
        SDL_SetStringProperty(props, "SDL_mixer.decoder.fluidsynth.soundfont_path", reinterpret_cast<const char*>(utf8.c_str()));
        candidate = MIX_LoadAudioWithProperties(props);
        SDL_DestroyProperties(props);
        if (io) { SDL_CloseIO(io); }
    }
    else
    {
        auto utf8 = path.value().u8string();
        candidate = MIX_LoadAudio(mixer, reinterpret_cast<const char*>(utf8.c_str()), true);
    }
    if (!candidate) { return Result<bool>::failure(make_error(SDL_GetError())); }
    auto stopped = stop_music();
    if (!stopped)
    {
        MIX_DestroyAudio(candidate);
        return stopped;
    }
    if (!MIX_SetTrackAudio(track, candidate))
    {
        MIX_DestroyAudio(candidate);
        return Result<bool>::failure(make_error(SDL_GetError()));
    }
    MIX_DestroyAudio(audio);
    audio = candidate;
    music_path_ = identity;
    music_midi_ = midi;
    auto volume = music_volume(midi ? midi_volume_ : cd_volume_);
    if (!volume) { return volume; }
    auto result = replay();
    if (result)
    {
        music_id_ = id;
#ifdef __ANDROID__
        SDL_Log("Music started: id=%u midi=%d duration=%lld", id, int(midi), static_cast<long long>(MIX_GetAudioDuration(audio)));
#endif
        if (diagnostics_) { std::print(stderr, "Original music id={} file={}\n", id, music_path_.filename().string()); }
    }
    return result;
}

auto OpeningAudio::mixer_handle() const -> MIX_Mixer* { return mixer; }

auto OpeningAudio::start_effects(const std::filesystem::path& root) -> Result<bool>
{
    auto candidate = std::make_unique<OriginalEffectAudio>();
    auto started = candidate->start(mixer, root);
    if (started) { effects_ = std::move(candidate); }
    return started;
}

auto OpeningAudio::effects() const -> OriginalEffectAudio* { return effects_.get(); }

OpeningAudio::OpeningAudio()
{
    for (int level = FLUID_PANIC; level <= FLUID_DBG; ++level) { fluid_set_log_function(level, fluid_log, nullptr); }
}

auto OpeningAudio::set_diagnostics(bool enabled) -> void { diagnostics_ = enabled; }

OpeningAudio::~OpeningAudio() { stop(); }

auto OpeningAudio::stop() -> void
{
    effects_.reset();
    xmi_resources_.clear();
    MIX_DestroyTrack(track);
    track = nullptr;
    MIX_DestroyAudio(audio);
    audio = nullptr;
    MIX_DestroyMixer(mixer);
    mixer = nullptr;
    if (initialized)
    {
        MIX_Quit();
        initialized = false;
    }
}

auto OpeningAudio::start(const std::filesystem::path& path) -> Result<bool>
{
    auto failure = [this](std::string message)
    {
        auto error = make_error(std::move(message));
        stop();
        return Result<bool>::failure(std::move(error));
    };
    if (!MIX_Init())
    {
        return failure(SDL_GetError());
    }
    initialized = true;
    SDL_AudioDeviceID chosen = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    const char* requested = SDL_getenv("S2NE_AUDIO_DEVICE");
    if (requested && *requested)
    {
        int count = 0;
        auto* devices = SDL_GetAudioPlaybackDevices(&count);
        for (int i = 0; devices && i < count; ++i)
        {
            const char* name = SDL_GetAudioDeviceName(devices[i]);
            if (name && std::string(name).find(requested) != std::string::npos)
            {
                chosen = devices[i];
                break;
            }
        }
        SDL_free(devices);
        if (chosen == SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK)
        {
            return failure("Requested S2NE_AUDIO_DEVICE was not found");
        }
    }
    mixer = MIX_CreateMixerDevice(chosen, nullptr);
    if (!mixer)
    {
        return failure(SDL_GetError());
    }
    if (!MIX_SetMixerGain(mixer, 1.f) || !MIX_SetPostMixCallback(mixer, monitor, this))
    {
        return failure(SDL_GetError());
    }
    if (diagnostics_)
    {
        std::print(stderr, "Audio driver: {}\n", SDL_GetCurrentAudioDriver());
        int device_count = 0;
        auto* devices = SDL_GetAudioPlaybackDevices(&device_count);
        for (int i = 0; devices && i < device_count; ++i)
        {
            std::print(stderr, "Playback device: {}\n", SDL_GetAudioDeviceName(devices[i]));
        }
        SDL_free(devices);
    }
    track = MIX_CreateTrack(mixer);
    if (!track || !MIX_SetTrackGain(track, 1.f))
    {
        return failure(SDL_GetError());
    }
    if (path.empty()) { return Result<bool>::success(true); }
    const auto utf8 = path.u8string();
    audio = MIX_LoadAudio(mixer, reinterpret_cast<const char*>(utf8.c_str()), true);
    if (!audio || MIX_GetAudioDuration(audio) <= 0 || !MIX_SetTrackAudio(track, audio))
    {
        return failure(SDL_GetError());
    }
    return replay();
}

auto OpeningAudio::replay() -> Result<bool>
{
    auto failure = [](std::string message)
    {
        return Result<bool>::failure(make_error(std::move(message)));
    };
    auto props = SDL_CreateProperties();
    if (!props)
    {
        return failure(SDL_GetError());
    }
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    bool ok = MIX_PlayTrack(track, props);
    SDL_DestroyProperties(props);
    if (!ok)
    {
        return failure(SDL_GetError());
    }
    return Result<bool>::success(true);
}

auto OpeningAudio::toggle() -> Result<bool>
{
    auto failure = [](std::string message)
    {
        return Result<bool>::failure(make_error(std::move(message)));
    };
    muted = !muted;
    if (!MIX_SetTrackGain(track, muted ? 0.f : music_volume_ / 100.f))
    {
        muted = !muted;
        return failure(SDL_GetError());
    }
    return Result<bool>::success(true);
}

auto OpeningAudio::is_muted() const -> bool { return muted; }

auto OpeningAudio::progressing() const -> bool
{
    std::print("Non-silent mixed samples: {}\n", signal_samples.load());
    return MIX_TrackPlaying(track) && MIX_GetTrackPlaybackPosition(track) > 0 && signal_samples.load() > 0;
}
}    //namespace s2ne
