#pragma once
#include "original_disc.hpp"
#include "original_presentation.hpp"
#include "resource_result.hpp"
#include "texture_resources.hpp"
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
inline Result<std::vector<uint8_t>> try_load_title(const std::filesystem::path& root, unsigned resource = 19)
{
    const auto external = root / "texture" / "opening" / (std::to_string(resource == 20 ? 19 : resource) + ".png");
    static std::filesystem::path cached_path;
    static int cached_width{}, cached_height{};
    static std::vector<uint8_t> cached_image;
    {
        if (cached_image.empty() || cached_path != external || cached_width != screen_width || cached_height != screen_height)
        {
            auto loaded = TextureResources::load_file_scaled(external, screen_width, screen_height);
            if (!loaded) { return Result<std::vector<uint8_t>>::failure(loaded.error()); }
            cached_image = loaded.value().pixels;
            cached_path = external;
            cached_width = screen_width;
            cached_height = screen_height;
        }
        std::vector<uint8_t> image = cached_image;
        if (resource == 20)
        {
            for (size_t p = 0; p < image.size(); p += 4)
            {
                for (unsigned c = 0; c < 3; ++c) { image[p + c] = uint8_t(unsigned(image[p + c]) * 30 / 100); }
            }
        }
        set_fixed_page_background(image);
        return Result<std::vector<uint8_t>>::success(fixed_page_center());
    }
}
inline std::vector<uint8_t> load_title(const std::filesystem::path& root, unsigned resource = 19)
{
    auto result = try_load_title(root, resource);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

}    //namespace s2ne
