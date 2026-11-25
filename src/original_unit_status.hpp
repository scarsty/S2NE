#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_sprite.hpp"
#include <memory>
#include <string>

namespace s2ne
{
// 4571EB draws over the caller's original panel background; panel_y is 0 or 363.
class OriginalUnitStatus
{
    SharedBytes cel_, pal_;
    size_t heads_{};
    mutable std::vector<uint8_t> cached_input_, cached_output_;
    mutable std::vector<uint8_t> cached_name_, cached_profession_;
    mutable std::array<int64_t, 26> cached_fields_{};
    mutable const OriginalMenu* cached_font_{};
    static unsigned word(const std::vector<uint8_t>& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const std::vector<uint8_t>& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    static s2ne::Result<SharedBytes> try_read(const std::filesystem::path& p) { return read_shared_resource(p); }
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
    s2ne::Result<bool> try_portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const
    {
        if (!id || size_t(id) * 2 > word(cel_, heads_))
        {
            return s2ne::Result<bool>::failure(make_error("Invalid original portrait ID"));
        }
        // 4461D5/445D38 select the second (small) image of each portrait pair.
        size_t p = heads_ + dword(cel_, heads_ + 2 + (size_t(id) * 2 - 1) * 8);
        return draw_original_sprite(image, cel_, pal_, p, true, x, y, OriginalZeroRun::transparent);
    }

    void portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const
    {
        auto result = try_portrait(image, id, x, y);
        if (!result)
        {
            report_error(result.error());
        }
    }
    static std::vector<uint8_t> bytes(const std::string& s) { return { s.begin(), s.end() }; }

    OriginalUnitStatus(SharedBytes cel, SharedBytes pal) : cel_(std::move(cel)), pal_(std::move(pal)) {}
    s2ne::Result<bool> initialize()
    {
        pal_.mutable_at(0) = pal_.mutable_at(1) = pal_.mutable_at(2) = 0;
        heads_ = dword(pal_, 808 + 2 * 8) - 58;
        if (heads_ + 2 > cel_.size())
        {
            return s2ne::Result<bool>::failure(make_error("Invalid original portrait archive"));
        }

        return s2ne::Result<bool>::success(true);
    }

public:
    explicit OriginalUnitStatus(const std::filesystem::path& root) : OriginalUnitStatus(std::move(*create(root).value()))
    {
    }
    static s2ne::Result<std::unique_ptr<OriginalUnitStatus>> create(const std::filesystem::path& root)
    {
        auto cel = try_read(root / "SUPERSP.CEL");
        if (!cel) { return s2ne::Result<std::unique_ptr<OriginalUnitStatus>>::failure(cel.error()); }
        auto pal = try_read(root / "SUPERSP.PAL");
        if (!pal) { return s2ne::Result<std::unique_ptr<OriginalUnitStatus>>::failure(pal.error()); }
        if (pal.value().size() < 828) { return s2ne::Result<std::unique_ptr<OriginalUnitStatus>>::failure(make_error("Truncated status palette")); }
        auto result = std::unique_ptr<OriginalUnitStatus>(new OriginalUnitStatus(std::move(cel).value(), std::move(pal).value()));
        auto ready = result->initialize();
        if (!ready)
        {
            return s2ne::Result<std::unique_ptr<OriginalUnitStatus>>::failure(ready.error());
        }
        return s2ne::Result<std::unique_ptr<OriginalUnitStatus>>::success(std::move(result));
    }

    // Returns the missing right-hand segment, exactly as IDIV in 4573A1 (no ratio clamp).
    static int missing_bar(int current, int limit)
    {
        return !current || !limit ? 107 : int((int64_t(limit) - current) * 107 / limit);
    }
    s2ne::Result<bool> try_draw_status_chain(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u,
        int x_base, int y_base) const
    {
        if (image.size() != 640 * 480 * 4)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid status framebuffer"));
        }
        const std::array<int, 4> current{ u.hp, u.mp, u.st, u.action_points };
        const std::array<int, 4> limit{ u.max_hp, u.max_mp, u.max_st, u.base_action_points };
        const std::array<int, 4> gate_current{ u.hp, u.mp, u.st, u.action_points };
        const std::array<int, 4> gate_limit{ u.max_hp, u.max_mp, u.max_st, u.base_action_points };
        const std::array<int, 4> y_offsets{ 6, 0x17, 0x28, 0x39 };
        const std::array<const char*, 4> labels{ "HP", "MP", "ST", "AP" };
        auto small = [&](const std::vector<uint8_t>& text, int x, int y, unsigned color)
        {
            return font.try_draw_small_validated(image, text, x, y, color, true);
        };
        for (unsigned row = 0; row < 4; ++row)
        {
            const int y = y_base + y_offsets[row];
            auto label = small(bytes(labels[row]), x_base + 0x0D, y, 187);
            if (!label) { return label; }
            auto value = small(bytes(std::to_string(current[row])), x_base + 0xB8, y, 79);
            if (!value) { return value; }
            auto separator = small(bytes("/"), x_base + 0xD8, y, 79);
            if (!separator) { return separator; }
            auto maximum = std::to_string(limit[row]);
            if (row == 3 && u.carry_points > 0) { maximum += "+" + std::to_string(u.carry_points); }
            auto maximum_text = small(bytes(maximum), x_base + 0xE0, y, 79);
            if (!maximum_text) { return maximum_text; }
            if (gate_current[row] < gate_limit[row] || !current[row])
            {
                const int gap = missing_bar(current[row], limit[row]);
                const int right = x_base + 0x9F;
                const int top = y + 5;
                rectangle(image, right - gap, top, right, top, 221);
                rectangle(image, right - gap, top + 1, right, top + 2, 43);
                rectangle(image, right - gap, top + 3, right, top + 5, 46);
            }
        }
        const std::array<unsigned, 6> labels_id{ 7, 8, 10, 11, 9, 12 };
        const std::array<int, 6> values{ u.attack, u.defense, u.strikeback, u.sp, u.tactics, u.fencing };
        for (unsigned i = 0; i < 6; ++i)
        {
            const int column = i < 3 ? 0 : 0x41;
            const int y = y_base + 9 + int(i % 3) * 0x16;
            auto label = small(font.text_bytes(labels_id[i]), x_base + 0x10B + column, y, 54);
            if (!label) { return label; }
            int adjustment = 0;
            if (i % 3 == 0) { adjustment = 1; }
            else if (i % 3 == 2) { adjustment = -1; }
            auto value = small(bytes(std::to_string(values[i])), x_base + 0x131 + column, y + adjustment, 202);
            if (!value) { return value; }
        }
        return s2ne::Result<bool>::success(true);
    }
    s2ne::Result<bool> try_draw_report(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u,
        int x_base, int y_base, bool alternate_context = false) const
    {
        if (image.size() != 640 * 480 * 4)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid status framebuffer"));
        }
        auto portrait_result = try_portrait(image, u.portrait, x_base + 0x15, y_base + 0x14);
        if (!portrait_result)
        {
            return portrait_result;
        }
        if (!(u.status_flags & 0x10))
        {
            auto name = font.try_draw_original_large(image, u.name, x_base + 0x82, y_base + 0x16, alternate_context ? 60 : 167);
            if (!name) { return name; }
        }
        auto title = u.profession;
        auto level = bytes(" " + std::to_string(u.level));
        title.insert(title.end(), level.begin(), level.end());
        auto suffix = font.text_bytes(113);
        title.insert(title.end(), suffix.begin(), suffix.end());
        auto heading = font.try_draw_original_large(image, title, x_base + 0x204 - int(title.size()) * 12, y_base + 0x16, 179);
        if (!heading) { return heading; }
        auto status = try_draw_status_chain(font, image, u, x_base + 0x7B, y_base + 0x36);
        if (!status)
        {
            return status;
        }
        return s2ne::Result<bool>::success(true);
    }
    s2ne::Result<bool> try_draw(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u, int panel_y,
        bool alternate_context = false) const
    {
        if (image.size() != 640 * 480 * 4)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid status framebuffer"));
        }
        auto portrait_result = try_portrait(image, u.portrait, 7, panel_y + 3);
        if (!portrait_result) { return portrait_result; }
        if (!(u.status_flags & 0x10))
        {
            auto name = font.try_draw_original_large(image, u.name, 111, panel_y + 7, alternate_context ? 60 : 167);
            if (!name) { return name; }
        }
        auto title = u.profession;
        const auto level = bytes(" " + std::to_string(u.level));
        title.insert(title.end(), level.begin(), level.end());
        const auto suffix = font.text_bytes(113);
        title.insert(title.end(), suffix.begin(), suffix.end());
        auto heading = font.try_draw_small_validated(image, title, 276, panel_y + 11, 178, true);
        if (!heading) { return heading; }
        auto label = font.try_draw_small_validated(image, font.text_bytes(4), 408, panel_y + 11, 178, true);
        if (!label) { return label; }
        auto experience = font.try_draw_small_validated(image, bytes(std::to_string(u.experience)), 449, panel_y + 11, 60, true);
        if (!experience) { return experience; }
        return try_draw_status_chain(font, image, u, 104, panel_y + 33);
    }
    void draw(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u, int panel_y, bool alternate_context = false) const
    {
        if (image.size() != 640u * 480 * 4 || (panel_y != 0 && panel_y != 363))
        {
            report_error(make_error("Invalid status framebuffer or panel position"));
            return;
        }
        const std::array<int64_t, 26> fields{ u.portrait, u.status_flags, u.level, u.experience,
            u.hp, u.mp, u.st, u.action_points, u.max_hp, u.max_mp, u.max_st, u.base_action_points,
            u.carry_points, u.attack, u.defense, u.strikeback, u.sp, u.tactics, u.fencing,
            panel_y, alternate_context, font.small_font, font.simplified_chinese,
            texture_resources.active(), texture_resources.font_ready(), texture_resources.small_font_ready() };
        const size_t offset = size_t(panel_y) * 640 * 4, extent = 117u * 640 * 4;
        const auto first = image.begin() + offset;
        if (!texture_resources.active() && cached_font_ == &font && !cached_output_.empty() && fields == cached_fields_ && u.name == cached_name_ && u.profession == cached_profession_
            && std::equal(cached_input_.begin(), cached_input_.end(), first))
        {
            std::copy(cached_output_.begin(), cached_output_.end(), first);
            return;
        }
        auto staged = image;
        auto result = try_draw(font, staged, u, panel_y, alternate_context);
        if (result)
        {
            cached_input_.assign(first, first + extent);
            cached_output_.assign(staged.begin() + offset, staged.begin() + offset + extent);
            cached_fields_ = fields;
            cached_font_ = &font;
            cached_name_ = u.name;
            cached_profession_ = u.profession;
            image.swap(staged);
        }
        else
        {
            report_error(result.error());
        }
    }
};
}    //namespace s2ne
