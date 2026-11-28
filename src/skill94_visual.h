#pragma once
#include "skill_visual.h"
namespace s2ne
{
// Sword94 only: BIN10 visual84 -> CEL1 SPELLSP85, 46B9A9 mode2.
class Skill94Visual
{
    std::vector<Skill32Visual::Frame> frames_;
    size_t frame_{};
    uint64_t elapsed_{};
    bool presented_{};

public:
    static Result<std::vector<Skill32Visual::Frame>> try_parse(const std::vector<uint8_t>& entry);
    Skill94Visual() = default;
    static std::vector<s2ne::Skill32Visual::Frame> parse(const std::vector<uint8_t>& entry);
    static Result<Skill94Visual> create(const std::filesystem::path& root);
    explicit Skill94Visual(const std::filesystem::path& root);
    const std::vector<s2ne::Skill32Visual::Frame>& frames() const;
    const s2ne::Skill32Visual::Frame& current() const;
    size_t frame() const;
    bool done() const;
    void presented();
    void tick(unsigned ms, bool accelerated = false);
};
}    //namespace s2ne
