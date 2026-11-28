#include "original_action_audio.h"

namespace s2ne
{
auto original_skill_sounds(const std::filesystem::path& root, unsigned cursor) -> Result<std::vector<uint8_t>>
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

auto OriginalActionAudioCues::begin(std::vector<Cue> cues) -> void
{
    std::stable_sort(cues.begin(), cues.end(), [](const Cue& a, const Cue& b)
        {
            return a.frame < b.frame;
        });
    cues_ = std::move(cues);
    next_ = 0;
}

auto OriginalActionAudioCues::cancel() -> void
{
    cues_.clear();
    next_ = 0;
}

auto original_movement_sound(unsigned mode, int appearanceType, int ground, unsigned terrain) -> unsigned
{
    if (mode == 4 || mode == 5) { return 132; }
    if (mode == 3) { return 133; }
    if (mode == 1) { return 122; }
    if (ground > 0 && ground <= 4 && (terrain == 1 || terrain == 2)) { return 131; }
    if (appearanceType == 4) { return 124; }
    return 103;
}
}    //namespace s2ne
