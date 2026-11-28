#include "original_combat_visual.h"

namespace s2ne
{
auto try_reaction_bitmap(unsigned width, unsigned height,
    std::span<const uint8_t> current, std::span<const uint8_t> overlap,
    std::span<const uint8_t> foot_mask, std::span<const uint8_t> shadow, bool left) -> Result<std::vector<uint8_t>>
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

auto reaction_bitmap(unsigned width, unsigned height, std::span<const uint8_t> current, std::span<const uint8_t> overlap, std::span<const uint8_t> foot_mask, std::span<const uint8_t> shadow, bool left) -> std::vector<uint8_t>
{
    auto result = try_reaction_bitmap(width, height, current, overlap, foot_mask, shadow, left);
    if (!result)
    {
        report_error(result.error());
        return {};
    }
    return std::move(result).value();
}

auto Disappearance::try_begin(unsigned id, std::span<const uint8_t> a, std::span<const uint8_t> b) -> Result<bool>
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

auto Disappearance::begin(unsigned id, std::span<const uint8_t> a, std::span<const uint8_t> b) -> void
{
    auto result = try_begin(id, a, b);
    if (!result)
    {
        report_error(result.error());
    }
}

auto Disappearance::enable_presentation_timing() -> void
{
    presentation_driven = true;
    presented_frame = false;
}

auto Disappearance::presented() -> void { presented_frame = true; }

auto Disappearance::active() const -> bool { return running; }

auto Disappearance::finished() const -> bool { return done; }

auto Disappearance::death_mode() -> void
{
    frame_count = 10;
    frame_interval = 30;
    first_weight = 10;
}

auto Disappearance::percentage() const -> unsigned { return first_weight + frame * 10; }

auto Disappearance::tick(unsigned ms) -> void
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

auto Feedback::active() const -> bool { return phase != Phase::idle; }

auto Feedback::silhouette() const -> bool { return phase == Phase::flash; }

auto Feedback::silhouette_palette() const -> unsigned { return 0x3c; }

auto Feedback::text_visible() const -> bool { return phase == Phase::numbers && step < 7; }

auto Feedback::text() const -> std::string { return miss ? "MISS" : std::to_string(std::abs(damage)); }

auto Feedback::text_x() const -> int { return 32 - int(text().size()) * 8; }

auto Feedback::text_y() const -> int { return -64 - step * 8; }

auto Feedback::text_foreground() const -> unsigned
{
    if (miss) { return 0x68; }
    return damage < 0 ? 0x24 : 0x3c;
}

auto Feedback::text_background() const -> unsigned
{
    if (miss) { return 0x24; }
    return damage < 0 ? 0xbe : 0x56;
}

auto Feedback::interval() const -> unsigned
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

auto Feedback::begin(size_t a, size_t b, int hp, int amount, bool missed, bool windup, int max_hp) -> void
{
    *this = {};
    attacker = a;
    target = b;
    damage = amount < 0 ? -std::min(std::max(0, max_hp - hp), -amount) : std::min(hp, amount);
    remaining = std::abs(damage);
    hp_before = hp_after = hp;
    miss = missed;
    phase = windup ? Phase::windup : Phase::flash;
}

auto Feedback::consume() -> int
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

auto Feedback::consume_with_caster(unsigned effect, int& caster_hp, int caster_max_hp, bool reverse) -> int
{
    const int chunk = consume();
    if (chunk <= 0) { return chunk; }
    if (effect == 41) { caster_hp = int16_t(uint16_t(caster_hp - chunk)); }
    else if (effect == 42 && caster_hp < caster_max_hp)
    {
        if (!reverse) { caster_hp = std::min(caster_max_hp, int(int16_t(uint16_t(caster_hp + chunk)))); }
    }
    else if (reverse)
    {
        caster_hp = std::max(0, int(int16_t(uint16_t(caster_hp - chunk))));
    }
    return chunk;
}
}    //namespace s2ne
