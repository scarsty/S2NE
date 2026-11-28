#pragma once
#include "original_palette.h"
#include "resource_result.h"
#include "texture_resources.h"
#include <algorithm>

namespace s2ne
{
// 474E3E arg_0 applies only to zero opcodes/repeats. Literals copy even index 0.
enum class OriginalZeroRun
{
    transparent,
    opaque
};
Result<bool> draw_original_sprite(std::vector<uint8_t>& image, std::span<const uint8_t> bytes,
    std::span<const uint8_t> palette, size_t position, bool typed, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed = nullptr);
Result<bool> draw_original_cel_legacy(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    std::span<const uint8_t> palette, unsigned id, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed = nullptr);
Result<bool> draw_original_panel_shadow(std::vector<uint8_t>& image, const std::vector<uint8_t>& palette,
    int width, int height, int x, int y, std::vector<uint8_t>* indexed = nullptr);
Result<bool> draw_original_cel_shadow(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    const std::vector<uint8_t>& palette, unsigned id, int x, int y, std::vector<uint8_t>* indexed = nullptr);
Result<bool> draw_original_cel(std::vector<uint8_t>& image, std::span<const uint8_t> cel,
    std::span<const uint8_t> palette, unsigned id, int left, int top, OriginalZeroRun zero, std::vector<uint8_t>* indexed = nullptr);
}    //namespace s2ne
