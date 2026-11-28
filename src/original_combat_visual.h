#pragma once
#include "runtime_error.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace s2ne
{
// 453238 consumes indexed AD/B1 buffers, not an RGBA framebuffer.
// The caller retains runtime40's draw-dependent alternating bit.
Result<std::vector<uint8_t>> try_reaction_bitmap(unsigned width, unsigned height,
    std::span<const uint8_t> current, std::span<const uint8_t> overlap,
    std::span<const uint8_t> foot_mask, std::span<const uint8_t> shadow, bool left);
std::vector<uint8_t> reaction_bitmap(unsigned width, unsigned height, std::span<const uint8_t> current, std::span<const uint8_t> overlap, std::span<const uint8_t> foot_mask, std::span<const uint8_t> shadow, bool left);

// 455048(ebx=0,ecx=0): five original indexed mixes, presented at 20ms.
// Gameplay presence is deliberately caller-owned until finished().
struct Disappearance
{
    unsigned target{};
    unsigned frame{}, elapsed{}, frame_count{ 5 }, frame_interval{ 20 }, first_weight{ 40 };
    bool running{}, done{};
    std::vector<uint8_t> source, background;
    Result<bool> try_begin(unsigned id, std::span<const uint8_t> a, std::span<const uint8_t> b);
    void begin(unsigned id, std::span<const uint8_t> a, std::span<const uint8_t> b);
    bool presentation_driven{}, presented_frame{};
    void enable_presentation_timing();
    void presented();
    bool active() const;
    bool finished() const;
    void death_mode();
    unsigned percentage() const;
    void tick(unsigned ms);
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
    size_t attacker{}, target{};
    int damage{}, remaining{}, hp_before{}, hp_after{}, step{};
    unsigned elapsed{};
    bool miss{};
    bool active() const;
    bool silhouette() const;
    unsigned silhouette_palette() const;
    bool text_visible() const;
    std::string text() const;
    int text_x() const;
    int text_y() const;
    unsigned text_foreground() const;
    unsigned text_background() const;
    unsigned interval() const;
    void begin(size_t a, size_t b, int hp, int amount, bool missed, bool windup, int max_hp = 0);
    // Called once for each original presented HP step; no RNG here.
    int consume();
    int consume_with_caster(unsigned effect, int& caster_hp, int caster_max_hp, bool reverse);
};
}    //namespace s2ne
