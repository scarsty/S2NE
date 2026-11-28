#include "music.h"

namespace s2ne
{
auto original_music_path(const std::filesystem::path& root, unsigned id, ResourceDomain domain) -> Result<std::filesystem::path>
{
    // 原41DA1B的两盘表为音轨序号；CD1/CD2.ini从音轨2开始列文件，不能将曲号直接当Axx。
    constexpr std::array<unsigned, 38> first{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0, 0, 0, 14, 15, 0, 0, 0, 0, 0, 16, 0, 17, 0, 0, 18, 19, 20, 0, 21, 0, 22, 23 };
    constexpr std::array<unsigned, 38> second{ 0, 17, 0, 0, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 19, 10, 11, 12, 13, 0, 14, 15, 0, 16, 0, 20, 21 };
    constexpr std::array<unsigned, 22> second_a{ 0, 15, 0, 0, 16, 0, 0, 0, 0, 0, 0, 18, 0, 0, 20, 21, 22, 2, 5, 17, 23, 24 };
    constexpr std::array<unsigned, 22> second_b{ 0, 0, 3, 4, 0, 6, 7, 8, 9, 10, 11, 0, 13, 14, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (!id || id >= first.size()) { return Result<std::filesystem::path>::failure(make_error("Invalid original music id")); }
    unsigned number = 0;
    char disc = 'A';
    unsigned index = second[id];
    if (domain == ResourceDomain::secondary && index)
    {
        number = second_a[index];
        if (!number)
        {
            disc = 'B';
            number = second_b[index];
        }
    }
    else if (domain == ResourceDomain::primary && first[id]) { number = first[id] + 1; }
    if (!number)
    {
        return Result<std::filesystem::path>::failure(make_error("Original music " + std::to_string(id) + " requires midisp XMI playback (no CD track)"));
    }
    char name[32];
    std::snprintf(name, sizeof(name), "SUPERSP_%c%02u.mp3", disc, number);
    return Result<std::filesystem::path>::success(root / "MP3" / name);
}
}    //namespace s2ne
