#pragma once
#include "original_presentation.hpp"
#include "runtime_error.hpp"
#include "texture_resources.hpp"
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
    const std::vector<uint8_t>& palette_bytes() const
    {
        if (borrowed_palette_) { return *borrowed_palette_; }
        return owned_palette_;
    }
    std::array<unsigned, 3> cycle_offsets_{};
    std::array<uint64_t, 3> cycle_times_{};
    int nearest(int red, int green, int blue) const
    {
        // sub_463165: cyclic G/R/B tree traversal, strict 32-unit bounds and distance < 11907.
        const std::array<int, 3> target{ green, red, blue };
        int best_distance = 11907, best = 257;
        std::vector<std::pair<int, int>> stack{ { -1, nodes_[0].right } };
        while (!stack.empty())
        {
            auto [axis, node] = stack.back();
            stack.pop_back();
            while (node != 257 && best_distance)
            {
                axis = (axis + 1) % 3;
                const auto& n = nodes_.at(size_t(node));
                if (n.rgb[axis] > target[axis] - 32)
                {
                    if (n.rgb[axis] <= target[axis] + 32)
                    {
                        stack.emplace_back(axis, n.right);
                        if (n.color >= 32 && n.color <= 254)
                        {
                            int distance = 0;
                            for (unsigned k = 0; k < 3; ++k)
                            {
                                int d = n.rgb[k] - target[k];
                                distance += d * d;
                            }
                            if (distance < best_distance)
                            {
                                best_distance = distance;
                                best = node;
                            }
                        }
                    }
                    node = n.left;
                }
                else
                {
                    node = n.right;
                }
            }
        }
        return nodes_.at(size_t(best)).color;
    }

public:
    uint8_t blend_index(unsigned from, unsigned to, int amount) const
    {
        if (from > 255 || to > 255)
        {
            fatal("Original palette blend index");
        }
        if (amount <= 0)
        {
            return uint8_t(from);
        }
        if (amount >= 100)
        {
            return uint8_t(to);
        }
        int rgb[3];
        for (unsigned k = 0; k < 3; ++k)
        {
            rgb[k] = (int(palette_bytes()[from * 3 + k]) * (100 - amount) + int(palette_bytes()[to * 3 + k]) * amount) / 100;
        }
        return uint8_t(nearest(rgb[0], rgb[1], rgb[2]));
    }
    explicit OriginalPalette(const SharedBytes& palette) : OriginalPalette(static_cast<const std::vector<uint8_t>&>(palette))
    {
        owned_palette_ = palette;
        borrowed_palette_ = nullptr;
    }
    explicit OriginalPalette(std::vector<uint8_t>&& palette) : OriginalPalette(static_cast<const std::vector<uint8_t>&>(palette))
    {
        owned_palette_ = SharedBytes(std::move(palette));
        borrowed_palette_ = nullptr;
    }
    explicit OriginalPalette(const std::vector<uint8_t>& palette) : borrowed_palette_(&palette)
    {
        if (palette.size() < 768)
        {
            fatal("Missing original palette");
        }
        // sub_46340F / sub_462ECB: center-nearest entry first, descending then ascending insertion.
        int center = 0, distance = 512;
        for (int c = 0; c < 256; ++c)
        {
            int d = 0;
            for (int k = 0; k < 3; ++k)
            {
                int v = int(palette[c * 3 + k]) - 31;
                d += v * v;
            }
            if (d < distance)
            {
                distance = d;
                center = c;
            }
        }
        nodes_[0].color = -1;
        nodes_[257].color = -1;
        int next = 1;
        auto insert = [&](int color)
        {
            int id = next++;
            auto& n = nodes_[id];
            n.rgb = { palette[color * 3 + 1], palette[color * 3], palette[color * 3 + 2] };
            n.color = color;
            int parent = 0, node = nodes_[0].right, axis = -1;
            bool right = true;
            while (node != 257)
            {
                parent = node;
                axis = (axis + 1) % 3;
                right = n.rgb[axis] >= nodes_[node].rgb[axis];
                node = right ? nodes_[node].right : nodes_[node].left;
            }
            (right ? nodes_[parent].right : nodes_[parent].left) = id;
        };
        for (int c = center; c >= 0; --c)
        {
            insert(c);
        }
        for (int c = center + 1; c < 256; ++c)
        {
            insert(c);
        }
        auto generate = [&](unsigned table, unsigned target, int amount)
        {
            for (unsigned c = 0; c < 256; ++c)
            {
                if (!amount)
                {
                    maps_[table][c] = uint8_t(c);
                    continue;
                }
                int rgb[3];
                for (unsigned k = 0; k < 3; ++k)
                {
                    rgb[k] = (int(palette[c * 3 + k]) * (100 - amount) + int(palette[target * 3 + k]) * amount) / 100;
                }
                int mapped = nearest(rgb[0], rgb[1], rgb[2]);
                if (mapped < 0 || mapped > 255)
                {
                    fatal("Original palette search failed");
                }
                maps_[table][c] = uint8_t(mapped);
            }
        };
        for (unsigned i = 0; i < maps_.size(); ++i) { generate(i, targets_[i], int(amounts_[i])); }
    }
    bool cycle_due(uint64_t elapsed_ms) const
    {
        constexpr std::array<unsigned, 3> periods{ 16, 9, 9 };
        const auto now = elapsed_ms / 10;
        for (unsigned i = 0; i < periods.size(); ++i)
        {
            if (now < cycle_times_[i] || now - cycle_times_[i] > periods[i]) { return true; }
        }
        return false;
    }
    void cycle_rect(std::vector<uint8_t>& image, int left, int top, int width, int height, uint64_t elapsed_ms)
    {
        // 472D78 table4EAF48: 3..10 at >16cs, 11..18 and 19..31 at >9cs.
        constexpr std::array<unsigned, 3> starts{ 3, 11, 19 }, counts{ 8, 8, 13 }, periods{ 16, 9, 9 };
        for (unsigned i = 0; i < 3; ++i)
        {
            uint64_t now = elapsed_ms / 10;
            if (now < cycle_times_[i])
            {
                cycle_times_[i] = now;
                cycle_offsets_[i] = 0;
            }
            else if (now - cycle_times_[i] > periods[i])
            {
                cycle_times_[i] = now;
                cycle_offsets_[i] = (cycle_offsets_[i] + 1) % counts[i];
            }
        }
        std::array<uint32_t, 256> source{};
        for (unsigned c = 0; c < 256; ++c)
        {
            for (unsigned k = 0; k < 3; ++k)
            {
                source[c] |= uint32_t(unsigned(palette_bytes()[c * 3 + k]) * 255 / 63) << (k * 8);
            }
        }
        std::array<uint32_t, 128> keys;
        keys.fill(0xFFFFFFFFu);
        std::array<unsigned, 128> rotations{};
        auto bucket = [](uint32_t rgb)
        {
            rgb ^= rgb >> 11;
            rgb *= 0x9E3779B1u;
            return size_t(rgb >> 25);
        };
        for (unsigned i = 0; i < 3; ++i)
        {
            for (unsigned c = starts[i]; c < starts[i] + counts[i]; ++c)
            {
                size_t slot = bucket(source[c]);
                while (keys[slot] != 0xFFFFFFFFu && keys[slot] != source[c]) { slot = (slot + 1) & 127; }
                if (keys[slot] != 0xFFFFFFFFu) { continue; }
                keys[slot] = source[c];
                rotations[slot] = starts[i] + (c - starts[i] + counts[i] - cycle_offsets_[i]) % counts[i];
            }
        }
        const int stride = int(image.size() / (480 * 4));
        const int right = std::min(stride, left + width);
        const int bottom = std::min(480, top + height);
        for (int y = std::max(0, top); y < bottom; ++y)
        {
            for (int x = std::max(0, left); x < right; ++x)
            {
                const size_t p = (size_t(y) * stride + x) * 4;
                uint32_t rgb = image[p] | uint32_t(image[p + 1]) << 8 | uint32_t(image[p + 2]) << 16;
                size_t slot = bucket(rgb);
                while (keys[slot] != 0xFFFFFFFFu && keys[slot] != rgb) { slot = (slot + 1) & 127; }
                if (keys[slot] == 0xFFFFFFFFu) { continue; }
                unsigned color = rotations[slot];
                for (unsigned k = 0; k < 3; ++k)
                {
                    image[p + k] = uint8_t(unsigned(palette_bytes()[color * 3 + k]) * 255 / 63);
                }
            }
        }
    }
    void cycle(std::vector<uint8_t>& image, uint64_t elapsed_ms) { cycle_rect(image, 0, 0, int(image.size() / (480 * 4)), 480, elapsed_ms); }
    uint8_t map(unsigned table, unsigned color) const { return maps_.at(table).at(color); }
    void transform(std::vector<uint8_t>& image, int first_row, int row_count, unsigned table) const
    {
        transform_rect(image, 0, first_row, 640, row_count, table);
    }
    Result<bool> try_transform_rect(std::vector<uint8_t>& destination, int left, int top, int width, int height, unsigned table, FullWidthCanvas* canvas = nullptr) const
    {
        const int stride = framebuffer_width(destination);
        if (!stride || table >= maps_.size())
        {
            return Result<bool>::failure(make_error("Invalid palette effect framebuffer or table"));
        }
        if (canvas && stride == 640 && left == 0 && width == 640)
        {
            return canvas->transform(destination, [&](auto& wide)
                {
                    return try_transform_rect(wide, 0, top, 768, height, table);
                });
        }
        for (int y = std::max(0, top); y < std::min(480, top + height); ++y)
        {
            for (int x = std::max(0, left); x < std::min(stride, left + width); ++x)
            {
                size_t p = (size_t(y) * stride + x) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    const unsigned target = unsigned(palette_bytes()[targets_[table] * 3 + k]) * 255 / 63;
                    destination[p + k] = uint8_t((unsigned(destination[p + k]) * (100 - amounts_[table]) + target * amounts_[table]) / 100);
                }
            }
        }
        return Result<bool>::success(true);
    }
    void transform_rect(std::vector<uint8_t>& image, int left, int top, int width, int height, unsigned table, FullWidthCanvas* canvas = nullptr) const
    {
        auto result = try_transform_rect(image, left, top, width, height, table, canvas);
        if (!result)
        {
            fatal(result.error());
        }
    }
};
}    //namespace s2ne
