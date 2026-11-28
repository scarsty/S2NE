#pragma once
#include "original_map.h"
#include "original_sprite.h"
#include <SDL3/SDL_stdinc.h>
#include <fstream>
namespace s2ne
{
class OriginalSpiritSight
{
    SharedBytes kinds_, cel_, pal_;
    static std::vector<uint8_t> text(const auto& bytes, size_t p, size_t limit)
    {
        std::vector<uint8_t> result;
        for (size_t n = 0; n < limit && bytes.at(p + n); ++n) { result.push_back(bytes.at(p + n)); }
        return result;
    }
    std::vector<uint8_t> kind_string(unsigned directory, unsigned index) const;

public:
    explicit OriginalSpiritSight(const std::filesystem::path& root);
    Result<std::vector<uint8_t>> screenshot_indexed_pcx(std::span<const uint8_t> indices) const;
    Result<std::vector<uint8_t>> screenshot_pcx(const std::vector<uint8_t>& image) const;
    Result<std::filesystem::path> save_screenshot(const std::filesystem::path& root, const std::vector<uint8_t>& image,
        std::optional<unsigned>& sequence, std::span<const uint8_t> indices = {}) const;
    Result<std::vector<uint8_t>> draw(std::vector<uint8_t> image, const OriginalMenu& font, const OriginalEvents& events,
        const OriginalEvents::Unit& target, int screen_x, std::vector<uint8_t>* indexed = nullptr) const;
};
}    //namespace s2ne
