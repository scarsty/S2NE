#pragma once
#include "original_disc.h"
#include "original_presentation.h"
#include "resource_result.h"
#include "texture_resources.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
namespace s2ne
{
// 472EF1: DAC byte << 2; 47F5D2: unsigned product / 255.
// Fade-in visits 0,4,...,60 (not 63) when ebx=-4.
inline constexpr uint8_t opening_palette_channel(uint8_t dac, unsigned fade = 60)
{
    return uint8_t(unsigned(uint8_t(unsigned(dac) << 2)) * unsigned(uint8_t(fade << 2)) / 255);
}
Result<std::vector<uint8_t>> try_load_title(const std::filesystem::path& root, unsigned resource = 19);
std::vector<uint8_t> load_title(const std::filesystem::path& root, unsigned resource = 19);

}    //namespace s2ne
