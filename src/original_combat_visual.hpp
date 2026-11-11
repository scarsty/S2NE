#pragma once
#include "runtime_error.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace s2ne::original_combat
{
// 453238 consumes indexed AD/B1 buffers, not an RGBA framebuffer.
// The caller retains runtime40's draw-dependent alternating bit.
inline Result<std::vector<uint8_t>> try_reaction_bitmap(unsigned width, unsigned height,
    std::span<const uint8_t> current, std::span<const uint8_t> overlap,
    std::span<const uint8_t> foot_mask, std::span<const uint8_t> shadow, bool left)
{
    if (!width || height < 32 || current.size() != size_t(width) * height || overlap.size() != current.size() || foot_mask.size() != 64 * 32 || shadow.size() != 256)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Original reaction indexed buffers"));
    }
    auto source = std::vector<uint8_t>(current.begin(), current.end());
    // 453334 fills transparent sprite pixels; it never replaces opaque art.
    int foot_left = int(width / 2) - 32;
    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            int dx = foot_left + x;
            if (dx < 0 || dx >= int(width))
            {
                continue;
            }
            size_t p = size_t(height - 32 + unsigned(y)) * width + unsigned(dx);
            if (foot_mask[size_t(y) * 64 + x] && !source[p])
            {
                source[p] = foot_mask[size_t(y) * 64 + x];
            }
        }
    }
    std::vector<uint8_t> result(size_t(128) * height);
    int offset = (128 - int(width)) / 2 - 4;
    // Original signed division truncates toward zero, unlike arithmetic shift.
    int source_x = offset < 0 && left ? -offset : 0;
    int count = offset < 0 ? int(width) + offset : int(width);
    int destination;
    if (offset < 0) { destination = 0; }
    else if (left) { destination = offset; }
    else
    {
        destination = 128 - offset - int(width);
    }

    for (unsigned y = 0; y < height; ++y)
    {
        for (int x = 0; x < count; ++x)
        {
            int dx = destination + x;
            if (dx < 0 || dx >= 128)
            {
                continue;
            }
            size_t p = size_t(y) * width + unsigned(source_x + x);
            result[size_t(y) * 128 + unsigned(dx)] = source[p] == 32 ? shadow[overlap[p]] : source[p];
        }
    }
    return Result<std::vector<uint8_t>>::success(std::move(result));
}
inline std::vector<uint8_t> reaction_bitmap(unsigned width, unsigned height, std::span<const uint8_t> current, std::span<const uint8_t> overlap, std::span<const uint8_t> foot_mask, std::span<const uint8_t> shadow, bool left)
{
    auto result = try_reaction_bitmap(width, height, current, overlap, foot_mask, shadow, left);
    if (!result)
    {
        report_error(result.error());
        return { };
    }
    return std::move(result).value();
}

// 455048(ebx=0,ecx=0): five original indexed mixes, presented at 20ms.
// Gameplay presence is deliberately caller-owned until finished().
struct Disappearance
{
    unsigned target{ };
    unsigned frame{ }, elapsed{ }, frame_count{ 5 }, frame_interval{ 20 }, first_weight{ 40 };
    bool running{ }, done{ };
    std::vector<uint8_t> source, background;
    Result<bool> try_begin(unsigned id, std::span<const uint8_t> a, std::span<const uint8_t> b)
    {
        if (a.empty() || a.size() != b.size())
        {
            return Result<bool>::failure(make_error("Original disappearance buffers"));
        }
        target = id;
        frame = elapsed = 0;
        running = true;
        done = false;
        presented_frame = false;
        source.assign(a.begin(), a.end());
        background.assign(b.begin(), b.end());
        return Result<bool>::success(true);
    }
    void begin(unsigned id, std::span<const uint8_t> a, std::span<const uint8_t> b)
    {
        auto result = try_begin(id, a, b);
        if (!result)
        {
            report_error(result.error());
        }
    }
    bool presentation_driven{ }, presented_frame{ };
    void enable_presentation_timing()
    {
        presentation_driven = true;
        presented_frame = false;
    }
    void presented() { presented_frame = true; }
    bool active() const { return running; }
    bool finished() const { return done; }
    void death_mode()
    {
        frame_count = 10;
        frame_interval = 30;
        first_weight = 10;
    }
    unsigned percentage() const { return first_weight + frame * 10; }
    void tick(unsigned ms)
    {
        if (!running)
        {
            return;
        }
        elapsed += std::min(ms, frame_interval);
        if (elapsed < frame_interval || (presentation_driven && !presented_frame))
        {
            return;
        }
        elapsed = 0;
        presented_frame = false;
        if (++frame == frame_count)
        {
            running = false;
            done = true;
        }
    }
    template <class Blend>
    std::vector<uint8_t> pixels(Blend blend) const
    {
        std::vector<uint8_t> result(source.size());
        for (size_t i = 0; i < source.size(); ++i)
        {
            if (source[i] && background[i])
            {
                result[i] = blend(source[i], background[i], int(percentage()));
            }
        }
        return result;
    }
};

// 452E7F and 43EB67/43EA08. Coordinates are relative to 42C50A's
// projected tile origin, not a fabricated particle/sprite animation.
struct Feedback
{
    enum class Phase
    {
        idle,
        windup,
        flash,
        numbers
    };
    Phase phase = Phase::idle;
    size_t attacker{ }, target{ };
    int damage{ }, remaining{ }, hp_before{ }, hp_after{ }, step{ };
    unsigned elapsed{ };
    bool miss{ };
    bool active() const { return phase != Phase::idle; }
    bool silhouette() const { return phase == Phase::flash; }
    unsigned silhouette_palette() const { return 0x3c; }
    bool text_visible() const { return phase == Phase::numbers && step < 7; }
    std::string text() const { return miss ? "MISS" : std::to_string(damage); }
    int text_x() const { return 32 - int(text().size()) * 8; }
    int text_y() const { return -64 - step * 8; }
    unsigned text_foreground() const { return miss ? 0x68 : 0x3c; }
    unsigned text_background() const { return miss ? 0x24 : 0x56; }
    unsigned interval() const
    {
        if (miss) { return 30u; }
        else if (step < 3) { return 30u; }
        else if (step < 4) { return 20u; }
        else if (step < 6) { return 10u; }
        else
        {
            return 0u;
        }
    }
    void begin(size_t a, size_t b, int hp, int amount, bool missed, bool windup, int max_hp = 0)
    {
        *this = { };
        attacker = a;
        target = b;
        damage = amount < 0 ? -std::min(std::max(0, max_hp - hp), -amount) : std::min(hp, amount);
        remaining = std::abs(damage);
        hp_before = hp_after = hp;
        miss = missed;
        phase = windup ? Phase::windup : Phase::flash;
    }
    // Called once for each original presented HP step; no RNG here.
    int consume()
    {
        if (miss)
        {
            return 0;
        }
        int chunk = std::min(remaining, int(uint32_t(1) << std::min(step, 30)));
        remaining -= chunk;
        if (damage < 0) { chunk = -chunk; }
        hp_after -= chunk;
        return chunk;
    }
};
}    //namespace s2ne::original_combat
