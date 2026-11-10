#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include <string>

namespace s2ne
{
// 4571EB draws over the caller's original panel background; panel_y is 0 or 363.
class OriginalUnitStatus
{
    std::vector<uint8_t> cel_, pal_;
    size_t heads_{ };
    static unsigned word(const std::vector<uint8_t>& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const std::vector<uint8_t>& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    static std::vector<uint8_t> read(const std::filesystem::path& p)
    {
        std::ifstream f(p, std::ios::binary);
        if (!f)
        {
            throw std::runtime_error("Missing original unit-status resource");
        }
        return { std::istreambuf_iterator<char>(f), { } };
    }
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned c) const
    {
        if (x < 0 || x >= 640 || y < 0 || y >= 480)
        {
            return;
        }
        size_t p = (size_t(y) * 640 + size_t(x)) * 4;
        for (unsigned k = 0; k < 3; ++k)
        {
            image.at(p + k) = uint8_t(unsigned(pal_.at(c * 3 + k)) * 255 / 63);
        }
        image.at(p + 3) = 255;
    }
    void rectangle(std::vector<uint8_t>& image, int x, int y, int right, int bottom, unsigned c) const
    {
        for (int yy = std::max(0, y); yy <= std::min(479, bottom); ++yy)
        {
            for (int xx = std::max(0, x); xx <= std::min(639, right); ++xx)
            {
                pixel(image, xx, yy, c);
            }
        }
    }
    void portrait(std::vector<uint8_t>& image, unsigned id, int y) const
    {
        if (!id || size_t(id) * 2 > word(cel_, heads_))
        {
            throw std::runtime_error("Invalid original portrait ID");
        }
        // 4461D5/445D38 select the second (small) image of each portrait pair.
        size_t p = heads_ + dword(cel_, heads_ + 2 + (size_t(id) * 2 - 1) * 8);
        unsigned type = cel_.at(p++), w = word(cel_, p) + 1, h = word(cel_, p + 2) + 1;
        p += 4;
        if (w > 640 || h > 480 || type > 1)
        {
            throw std::runtime_error("Invalid original status portrait");
        }
        for (unsigned yy = 0; yy < h; ++yy)
        {
            unsigned x = 0;
            while (x < w)
            {
                if (type)
                {
                    unsigned c = cel_.at(p++);
                    if (c)
                    {
                        pixel(image, 7 + int(x), y + int(yy), c);
                    }
                    ++x;
                    continue;
                }
                unsigned code = cel_.at(p++), n = code & 127;
                if (!code)
                {
                    ++x;
                    continue;
                }
                if (!n || n > w - x)
                {
                    throw std::runtime_error("Invalid original portrait run");
                }
                unsigned repeat = code & 128 ? 0 : cel_.at(p++);
                while (n--)
                {
                    unsigned c = code & 128 ? cel_.at(p++) : repeat;
                    if (c || (code & 128))
                    {
                        pixel(image, 7 + int(x), y + int(yy), c);
                    }
                    ++x;
                }
            }
        }
    }
    static std::vector<uint8_t> bytes(const std::string& s) { return { s.begin(), s.end() }; }

public:
    explicit OriginalUnitStatus(const std::filesystem::path& root) : cel_(read(root / "SUPERSP.CEL")), pal_(read(root / "SUPERSP.PAL"))
    {
        pal_.at(0) = pal_.at(1) = pal_.at(2) = 0;
        heads_ = dword(pal_, 808 + 2 * 8) - 58;
        if (heads_ + 2 > cel_.size())
        {
            throw std::runtime_error("Invalid original portrait archive");
        }
    }
    // Returns the missing right-hand segment, exactly as IDIV in 4573A1 (no ratio clamp).
    static int missing_bar(int current, int limit)
    {
        return !current || !limit ? 107 : int((int64_t(limit) - current) * 107 / limit);
    }
    void draw(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u, int panel_y, bool alternate_context = false) const
    {
        if (image.size() != 640 * 480 * 4)
        {
            throw std::runtime_error("Invalid status framebuffer");
        }
        portrait(image, u.portrait, panel_y + 3);
        if (!(u.status_flags & 0x10))
        {
            font.draw_original_large(image, u.name, 111, panel_y + 7, alternate_context ? 60 : 167);
        }
        auto small = [&](const std::vector<uint8_t>& text, int x, int y, unsigned color)
        {
            font.draw_small(image, text, x, y, color, true);
        };
        auto title = u.profession;
        auto level = bytes(" " + std::to_string(u.level));
        title.insert(title.end(), level.begin(), level.end());
        auto suffix = font.text_bytes(113);
        title.insert(title.end(), suffix.begin(), suffix.end());
        small(title, 276, panel_y + 11, 178);
        small(font.text_bytes(4), 409, panel_y + 12, 54);
        small(font.text_bytes(4), 408, panel_y + 11, 178);
        auto number = [&](int value, int x, int y, unsigned color)
        {
            small(bytes(std::to_string(value)), x, panel_y + y, color);
        };
        number(int(u.experience), 449, 11, 60);
        const std::array<int, 4> current{ u.hp, u.mp, u.st, u.max_action_points };
        const std::array<int, 4> limit{ u.max_hp, u.max_mp, u.max_st, u.base_action_points };
        // The original AP gate compares 12Eh to BFh, but the ratio uses 12Eh/BFh too.
        const std::array<int, 4> gate_current{ u.hp, u.mp, u.st, u.max_action_points };
        const std::array<int, 4> gate_limit{ u.max_hp, u.max_mp, u.max_st, u.base_action_points };
        const std::array<const char*, 4> labels{ "HP", "MP", "ST", "AP" };
        for (unsigned row = 0; row < 4; ++row)
        {
            int y = 39 + int(row) * 17;
            small(bytes(labels[row]), 117, panel_y + y, 187);
            number(current[row], 288, y, 79);
            small(bytes("/"), 320, panel_y + y, 79);
            if (row == 3 && u.carry_points > 0)
            {
                small(bytes(std::to_string(u.base_action_points) + "+" + std::to_string(u.carry_points)), 328, panel_y + y, 79);
            }
            else
            {
                number(row == 3 ? u.base_action_points : limit[row], 328, y, 79);
            }
            if (gate_current[row] < gate_limit[row] || !current[row])
            {
                int gap = missing_bar(current[row], limit[row]), right = 263, top = panel_y + 44 + int(row) * 17;
                rectangle(image, right - gap, top, right, top, 221);
                rectangle(image, right - gap, top + 1, right, top + 2, 43);
                rectangle(image, right - gap, top + 3, right, top + 5, 46);
            }
        }
        const std::array<unsigned, 6> label{ 7, 8, 10, 11, 9, 12 };
        const std::array<int, 6> value{ u.attack_limit, u.defense_limit, u.strikeback, u.sp, u.tactics, u.fencing };
        for (unsigned i = 0; i < 6; ++i)
        {
            int x = i < 3 ? 371 : 436, y = 42 + int(i % 3) * 22;
            small(font.text_bytes(label[i]), x, panel_y + y, 54);
            number(value[i], i < 3 ? 409 : 475, y + (i % 3 == 2 ? -1 : i % 3 == 0 ? 1 :
                                                                                    0),
                202);
        }
    }
};
}    //namespace s2ne
