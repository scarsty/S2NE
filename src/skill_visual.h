#pragma once
#include "palette.h"
#include "spell_resource.h"
#include <filesystem>
#include <fstream>
#include <iterator>

namespace s2ne
{
// Bounded 463D6D/463EA6 reader: spell32 selects CEL1 SPELLSP entry33.
class Skill32Visual
{
public:
    struct Frame
    {
        unsigned width{}, height{};
        std::vector<uint8_t> pixels;
        unsigned texture_visual{}, texture_frame{};
    };

private:
    std::vector<Frame> frames_;
    unsigned duration_{};
    std::array<int, 256> amounts_{};
    bool curse_{};
    size_t frame_{};
    uint64_t elapsed_{};
    bool presented_{};

public:
    static Result<Frame> try_decode(const std::vector<uint8_t>& bytes, unsigned expected_width = 63, unsigned expected_height = 272);
    static Result<std::vector<Frame>> try_parse(const std::vector<uint8_t>& entry);
    static Frame decode(const std::vector<uint8_t>& bytes, unsigned width = 63, unsigned height = 272);
    static std::vector<Frame> parse(const std::vector<uint8_t>& entry);
    Skill32Visual() = default;
    static Result<Skill32Visual> create(const std::filesystem::path& root);
    static Result<Skill32Visual> create_curse33(const std::filesystem::path& root);
    bool curse() const;
    int amount(unsigned color) const;
    explicit Skill32Visual(const std::filesystem::path& root);
    const std::vector<Frame>& frames() const;
    const Frame& current() const;
    size_t frame() const;
    bool done() const;
    unsigned duration_ms() const;
    void presented();
    void tick(unsigned milliseconds, bool accelerated = false);
    static void tint(std::vector<uint8_t>& indexed, const Palette& palette);
};
}    //namespace s2ne
