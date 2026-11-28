#pragma once
#include "original_skill_visual.h"
namespace s2ne
{
// Sword94 only: BIN10 visual84 -> CEL1 SPELLSP85, 46B9A9 mode2.
class OriginalSkill94Visual
{
    std::vector<OriginalSkill32Visual::Frame> frames_;
    size_t frame_{};
    uint64_t elapsed_{};
    bool presented_{};

public:
    static Result<std::vector<OriginalSkill32Visual::Frame>> try_parse(const std::vector<uint8_t>& entry);
    OriginalSkill94Visual() = default;
    static std::vector<s2ne::OriginalSkill32Visual::Frame> parse(const std::vector<uint8_t>& entry);
    static Result<OriginalSkill94Visual> create(const std::filesystem::path& root);
    explicit OriginalSkill94Visual(const std::filesystem::path& root);
    const std::vector<s2ne::OriginalSkill32Visual::Frame>& frames() const;
    const s2ne::OriginalSkill32Visual::Frame& current() const;
    size_t frame() const;
    bool done() const;
    void presented();
    void tick(unsigned ms, bool accelerated = false);
};
}    //namespace s2ne
