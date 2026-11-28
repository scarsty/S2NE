#pragma once
#include "native_ability_buffs.h"
#include "native_damage_records.h"
#include "native_timed_buffs.h"
#include "original_skill_visual.h"
namespace s2ne
{
class OriginalSkill88Visual
{
    std::vector<OriginalSkill32Visual::Frame> frames_;
    std::array<int, 256> amounts_{};
    size_t frame_{};
    uint64_t elapsed_{};
    int anchor_x_{ 89 }, anchor_y_{ 124 };
    unsigned tint_amount_{ 50 }, tint_source_{ 233 };
    bool presented_{};
    unsigned frame_delay_{ 50 };
    unsigned prefix_frames_{};
    unsigned texture_visual_{};
    size_t texture_frames_{};

public:
    static std::vector<uint8_t> shake_frame(const std::vector<uint8_t>& source, const std::vector<uint8_t>& previous, unsigned remaining);
    OriginalSkill88Visual() = default;
    static Result<OriginalSkill88Visual> create(const std::filesystem::path& root, bool healing1 = false, bool blessing3 = false);
    static Result<OriginalSkill88Visual> create_sleep2(const std::filesystem::path& root);
    static Result<OriginalSkill88Visual> create_strengthening_status(const std::filesystem::path& root, unsigned skill, unsigned cursor = 0);
    unsigned tint_source() const;
    unsigned texture_visual() const;
    size_t texture_frame(size_t timeline) const;
    explicit OriginalSkill88Visual(const std::filesystem::path& root);
    int anchor_x() const;
    int anchor_y() const;
    unsigned tint_amount() const;
    const std::vector<s2ne::OriginalSkill32Visual::Frame>& frames() const;
    const s2ne::OriginalSkill32Visual::Frame& current() const;
    const s2ne::OriginalSkill32Visual::Frame& frame_at(size_t index) const;
    unsigned frame_delay() const;
    void prepend_timeline(unsigned count);
    void extend_timeline(unsigned count);
    int amount(unsigned color) const;
    size_t frame() const;
    bool done() const;
    void presented();
    void tick(unsigned ms, bool accelerated = false);
};
}    //namespace s2ne
