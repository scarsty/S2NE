#include "original_effect_audio.h"

namespace s2ne
{
auto original_effect_channel(unsigned id, unsigned requested, unsigned lastId, int last,
    std::span<const bool> loaded, std::span<const bool> playing) -> int
{
    if (requested && requested <= loaded.size() && loaded[requested - 1]) { return int(requested - 1); }
    if (id == lastId && last >= 0 && size_t(last) < loaded.size() && loaded[size_t(last)] && !playing[size_t(last)]) { return last; }
    for (size_t i = 0; i < playing.size(); ++i)
    {
        if (!playing[i]) { return int(i); }
    }
    return -1;
}

auto OriginalEffectAudio::number(std::span<const uint8_t> b, size_t p, unsigned n) -> unsigned
{
    unsigned v = 0;
    for (unsigned i = 0; i < n; ++i) { v |= unsigned(b[p + i]) << (8 * i); }
    return v;
}

OriginalEffectAudio::~OriginalEffectAudio()
{
    for (auto* track : tracks_) { MIX_DestroyTrack(track); }
    for (auto* audio : decoded_) { MIX_DestroyAudio(audio); }
}

auto OriginalEffectAudio::start(MIX_Mixer* mixer, const std::filesystem::path& root) -> Result<bool>
{
    if (!mixer) { return Result<bool>::failure(make_error("Effects audio device unavailable")); }
    auto pal = read_resource(root / "SUPERSP.PAL");
    auto bin = read_shared_resource(root / "SUPERSP.BIN");
    if (!pal) { return Result<bool>::failure(pal.error()); }
    if (!bin) { return Result<bool>::failure(bin.error()); }
    const auto& p = pal.value();
    if (p.size() < 988) { return Result<bool>::failure(make_error("Truncated soundsp PAL index")); }
    const unsigned bias = 27;
    const auto encodedOffset = number(p, 848 + 16 * 8), encodedSize = number(p, 852 + 16 * 8);
    if (encodedOffset < bias || encodedSize < bias) { return Result<bool>::failure(make_error("Invalid soundsp span")); }
    const size_t offset = encodedOffset - bias, size = encodedSize - bias;
    if (offset > bin.value().size() || size > bin.value().size() - offset) { return Result<bool>::failure(make_error("Truncated soundsp")); }
    sounds_.assign(bin.value().begin() + offset, bin.value().begin() + offset + size);
    if (sounds_.size() < 1962 || number(sounds_, 0, 2) != 245) { return Result<bool>::failure(make_error("Invalid soundsp directory")); }
    for (unsigned id = 1; id <= 245; ++id)
    {
        const auto a = number(sounds_, 2 + (id - 1) * 8), n = number(sounds_, 6 + (id - 1) * 8);
        if (a > sounds_.size() || n > sounds_.size() - a || n < 12) { return Result<bool>::failure(make_error("Invalid original WAV extent")); }
    }
    mixer_ = mixer;
    for (auto& track : tracks_)
    {
        track = MIX_CreateTrack(mixer_);
        if (!track) { return Result<bool>::failure(make_error(SDL_GetError())); }
    }
    return Result<bool>::success(true);
}

auto OriginalEffectAudio::volume(int value) -> Result<bool>
{
    if (value < 0 || value > 100) { return Result<bool>::failure(make_error("Invalid original effect volume")); }
    volume_ = value;
    for (unsigned i = 0; i < tracks_.size(); ++i)
    {
        if (tracks_[i] && loaded_[i] && !MIX_SetTrackGain(tracks_[i], float(value) / 127.f)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    }
    return Result<bool>::success(true);
}

auto OriginalEffectAudio::play(unsigned id, unsigned channel) -> Result<unsigned>
{
    if (!mixer_) { return Result<unsigned>::failure(make_error("Effects audio unavailable")); }
    if (!id || id > 245) { return Result<unsigned>::failure(make_error("Original sound id error")); }
    if (!volume_) { return Result<unsigned>::success(0); }
    std::array<bool, 16> loaded{}, playing{};
    for (unsigned i = 0; i < 16; ++i)
    {
        loaded[i] = loaded_[i] != 0;
        playing[i] = MIX_TrackPlaying(tracks_[i]);
    }
    const int selected = original_effect_channel(id, channel, last_id_, last_, loaded, playing);
    if (selected < 0) { return Result<unsigned>::success(0); }
    const bool reuse = (channel && channel <= 16 && loaded[channel - 1]) || (id == last_id_ && last_ == selected && loaded[selected] && !playing[selected]);
    if (!reuse)
    {
        if (!decoded_[id - 1])
        {
            const auto offset = number(sounds_, 2 + (id - 1) * 8), size = number(sounds_, 6 + (id - 1) * 8);
            auto* io = SDL_IOFromConstMem(sounds_.data() + offset, size);
            if (!io) { return Result<unsigned>::failure(make_error(SDL_GetError())); }
            decoded_[id - 1] = MIX_LoadAudio_IO(mixer_, io, true, true);
            if (!decoded_[id - 1]) { return Result<unsigned>::failure(make_error(SDL_GetError())); }
        }
        if (!MIX_SetTrackAudio(tracks_[selected], decoded_[id - 1]) || !MIX_SetTrackGain(tracks_[selected], float(volume_) / 127.f)) { return Result<unsigned>::failure(make_error(SDL_GetError())); }
        loaded_[selected] = id;
    }
    if (!playing[selected] && !MIX_PlayTrack(tracks_[selected], 0)) { return Result<unsigned>::failure(make_error(SDL_GetError())); }
    last_id_ = id;
    last_ = selected;
    return Result<unsigned>::success(unsigned(selected + 1));
}

auto OriginalEffectAudio::stop() -> Result<bool>
{
    for (auto* track : tracks_)
    {
        if (track && !MIX_StopTrack(track, 0)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    }
    loaded_.fill(0);
    last_id_ = 0;
    last_ = -1;
    return Result<bool>::success(true);
}
}    //namespace s2ne
