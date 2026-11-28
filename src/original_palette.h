#pragma once
#include "original_presentation.h"
#include "runtime_error.h"
#include "texture_resources.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace s2ne
{
class OriginalPalette
{
    inline static constexpr std::array<unsigned, 13> targets_{ 33, 33, 33, 33, 33, 32, 32, 234, 32, 189, 36, 60, 68 };
    inline static constexpr std::array<unsigned, 13> amounts_{ 0, 10, 20, 30, 40, 60, 50, 65, 25, 60, 15, 15, 80 };
    struct Node
    {
        std::array<int, 3> rgb{};
        int color = -1, left = 257, right = 257;
    };
    std::array<Node, 258> nodes_{};
    std::array<std::array<uint8_t, 256>, 13> maps_{};
    SharedBytes owned_palette_;
    const std::vector<uint8_t>* borrowed_palette_{};
    const std::vector<uint8_t>& palette_bytes() const;
    std::array<unsigned, 3> cycle_offsets_{};
    std::array<uint64_t, 3> cycle_times_{};
    int nearest(int red, int green, int blue) const;

public:
    uint8_t blend_index(unsigned from, unsigned to, int amount) const;
    explicit OriginalPalette(const SharedBytes& palette);
    explicit OriginalPalette(std::vector<uint8_t>&& palette);
    explicit OriginalPalette(const std::vector<uint8_t>& palette);
    bool cycle_due(uint64_t elapsed_ms) const;
    void cycle_rect(std::vector<uint8_t>& image, int left, int top, int width, int height, uint64_t elapsed_ms);
    void cycle(std::vector<uint8_t>& image, uint64_t elapsed_ms);
    uint8_t map(unsigned table, unsigned color) const;
    void transform(std::vector<uint8_t>& image, int first_row, int row_count, unsigned table) const;
    Result<bool> try_transform_rect(std::vector<uint8_t>& destination, int left, int top, int width, int height, unsigned table, FullWidthCanvas* canvas = nullptr) const;
    void transform_rect(std::vector<uint8_t>& image, int left, int top, int width, int height, unsigned table, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
