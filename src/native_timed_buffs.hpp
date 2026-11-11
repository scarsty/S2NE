#pragma once
#include <array>
#include <cstdint>
namespace s2ne
{
// 4668F0 reverse repne table: these are effect operands, not switch indexes.
struct NativeTimedBuffs
{
    struct Oracle
    {
        unsigned id, group, bit, mask, visual, color, tint;
        std::array<uint8_t, 24> tail;
        std::array<unsigned, 8> header;
    };
    static constexpr std::array<Oracle, 6> records{ { { 37, 3, 4, 8, 37, 245, 50, { 18, 0, 56, 0, 0, 1, 62, 0, 0, 0, 100, 0, 3, 0, 0, 0, 0, 0, 1, 0, 1, 13, 9, 0 }, { 6, 0, 0, 97, 477, 0, 24, 4 } },
        { 44, 4, 1, 0x80, 44, 59, 25, { 18, 0, 10, 0, 0, 1, 70, 0, 0, 0, 100, 0, 3, 0, 0, 0, 0, 0, 1, 0, 1, 47, 19, 0 }, { 15, 147, 120, 290, 173, 0, 0, 5 } },
        { 46, 4, 3, 0x100, 46, 138, 50, { 15, 0, 32, 0, 4, 1, 72, 0, 120, 0, 100, 0, 5, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 }, { 34, 0, 0, 173, 173, 0, 36, 5 } },
        { 58, 5, 5, 0x200, 60, 233, 20, { 30, 0, 90, 0, 4, 1, 76, 0, 0, 0, 100, 0, 125, 0, 0, 0, 0, 0, 1, 0, 1, 53, 13, 0 }, { 43, 0, 0, 110, 110, 0, 0, 5 } },
        { 86, 8, 3, 0x400, 76, 233, 45, { 12, 0, 32, 0, 0, 1, 85, 0, 0, 0, 100, 0, 4, 0, 0, 0, 0, 0, 0, 0, 66, 49, 17, 0 }, { 40, 0, 0, 145, 119, 4, 53, 5 } },
        { 92, 8, 9, 0x800, 82, 137, 60, { 50, 0, 24, 1, 0, 1, 87, 0, 0, 0, 100, 0, 2, 0, 0, 0, 0, 0, 0, 0, 70, 34, 85, 0 }, { 27, 65535, 65535, 640, 480, 1, 50, 5 } } } };
    static constexpr const Oracle* find(unsigned id)
    {
        for (const auto& r : records)
        {
            if (r.id == id) { return &r; }
        }
        return nullptr;
    }
};
}    //namespace s2ne
