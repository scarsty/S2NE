#pragma once
#include "runtime_error.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace s2ne
{
// Raw on-disk state, not an OriginalEvents::Unit snapshot. See save-evidence.txt.
class Save
{
public:
    static constexpr unsigned save_address = 0x4362B4, load_address = 0x4367CB;
    static constexpr size_t unit_size = 0x16A, maximum_units = 50, fixed_size = 646;
    static constexpr std::array<uint8_t, 9> signature{ 0xA5, 0x5F, 0xA4, 0xE8, 0xB1, 0x4B, 0xA8, 0xCF, 0 };
    using UnitRecord = std::array<uint8_t, unit_size>;
    struct Group
    {
        std::vector<UnitRecord> units;
        // Runtime +CC (4), +DE (4), +D6 (4), +D4 (2), +E2 (4), in that order.
        std::array<uint8_t, 18> metadata{};
        std::vector<uint8_t> inventory;      // ITEMSP.BIN's signed 16-bit entry count, not its file size.
        std::array<uint8_t, 33> status{};    // runtime +286
        std::vector<int> role_selectors() const;
    };
    struct State
    {
        std::array<uint8_t, 50> description{};
        std::array<uint8_t, 255> attribution{};    // Preserve original stack-buffer padding verbatim.
        std::array<uint8_t, 80> flags_4f7d10{};
        std::array<uint8_t, 31> settings_4f7cf0{};
        std::array<uint8_t, 29> globals_4f7cd0{};
        std::array<uint8_t, 4> value_4f705c{};
        std::array<uint8_t, 160> words_4f6fbc{};
        std::array<uint8_t, 28> stage_4fb478{};
        std::array<Group, 2> groups;
        std::vector<uint16_t> event_states;
        bool battle_context() const;
    };

private:
    static uint32_t u32(std::span<const uint8_t> b, size_t p);
    [[noreturn]] static void fail();
    static void check_items(size_t count);
    class Reader
    {
        std::span<const uint8_t> bytes_;
        size_t position_ = 0;

    public:
        explicit Reader(std::span<const uint8_t> bytes);
        void take(std::span<uint8_t> dest);
        uint32_t dword();
        uint16_t word();
        size_t remaining() const;
    };
    static void append(std::vector<uint8_t>& out, std::span<const uint8_t> b);
    static void number(std::vector<uint8_t>& out, uint32_t value, unsigned width);
    static void validate(const State& s, size_t items);

public:
    static size_t item_count(std::span<const uint8_t> itemsp);
    // Validates framing only; unit pointer fixups/derived runtime fields are deliberately not applied.
    static Result<State> try_decode(std::span<const uint8_t> bytes, size_t items);
    static State decode(std::span<const uint8_t> bytes, size_t items);
    static State decode_validated(std::span<const uint8_t> bytes, size_t items);
    static Result<std::vector<uint8_t>> try_encode(const State& s, size_t items);
    static std::vector<uint8_t> encode(const State& s, size_t items);
    static std::vector<uint8_t> encode_validated(const State& s, size_t items);
    static Result<State> try_load(const std::filesystem::path& path, size_t items);
    static State load(const std::filesystem::path& path, size_t items);
    static std::filesystem::path slot_path(const std::filesystem::path& root, unsigned slot);
};
}    //namespace s2ne
