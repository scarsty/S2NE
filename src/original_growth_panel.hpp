#pragma once
#include "original_progression.hpp"
#include "original_shop_menu.hpp"
namespace s2ne
{
class OriginalGrowthPanel
{
public:
    static Result<std::vector<uint8_t>> draw(const std::filesystem::path& root, std::vector<uint8_t> image, const OriginalProgression::GrowthStep& step)
    {
        auto cel = read_resource(root / "SUPERSP.CEL");
        auto pal = read_resource(root / "SUPERSP.PAL");
        if (!cel) { return Result<std::vector<uint8_t>>::failure(cel.error()); }
        if (!pal) { return Result<std::vector<uint8_t>>::failure(pal.error()); }
        auto word = [](const auto& b, size_t p)
        {
            return unsigned(b.at(p)) | unsigned(b.at(p + 1)) << 8;
        };
        auto dword = [&](const auto& b, size_t p)
        {
            return word(b, p) | word(b, p + 2) << 16;
        };
        const size_t group = dword(pal.value(), 816) - 57;
        const size_t panel = group + dword(cel.value(), group + 2 + 97 * 8);
        const int width = int(word(cel.value(), panel)) + 1, height = int(word(cel.value(), panel + 2)) + 1;
        const int x = 320 - width / 2, y = 240 - height / 2;
        OriginalPalette palette(pal.value());
        for (int row = y + 8; row < y + height + 6; ++row)
        {
            int right = x;
            if (row >= y + height - 2) { right = x - 8 + width; }
            for (int col = x - 8; col < right; ++col)
            {
                if (col < 0 || col >= 640 || row < 0 || row >= 480) { continue; }
                const size_t pixel = (size_t(row) * 640 + col) * 4;
                for (unsigned c = 0; c < 256; ++c)
                {
                    if (image[pixel] != unsigned(pal.value()[c * 3]) * 255 / 63 || image[pixel + 1] != unsigned(pal.value()[c * 3 + 1]) * 255 / 63 || image[pixel + 2] != unsigned(pal.value()[c * 3 + 2]) * 255 / 63) { continue; }
                    auto mapped = palette.map(6, c);
                    for (unsigned k = 0; k < 3; ++k) { image[pixel + k] = uint8_t(unsigned(pal.value()[mapped * 3 + k]) * 255 / 63); }
                    break;
                }
            }
        }
        auto background = draw_original_cel(image, cel.value(), pal.value(), 239, x, y, OriginalZeroRun::transparent);
        if (!background) { return Result<std::vector<uint8_t>>::failure(background.error()); }
        OriginalMenu font(root);
        OriginalShopMenu graphics(root);
        auto text = step.before.name;
        text.push_back(' ');
        auto label = font.text_bytes(5);
        text.insert(text.end(), label.begin(), label.end());
        auto number = std::to_string(step.before.level);
        text.push_back(' ');
        text.insert(text.end(), number.begin(), number.end());
        auto title = font.try_draw_original_large(image, text, x + 27, y + 31, 42, 54);
        if (!title) { return Result<std::vector<uint8_t>>::failure(title.error()); }
        const auto& u = step.before;
        const std::array<int, 9> values{ u.max_hp, u.max_mp, u.max_st, u.attack_limit, u.defense_limit, u.strikeback, u.base_tactics, u.max_sp, u.fencing };
        const std::array<unsigned, 9> labels{ 0, 0, 0, 7, 8, 10, 9, 11, 12 };
        int row = y + 64;
        for (unsigned i = 0; i < 9; ++i)
        {
            if (!step.increments[i]) { continue; }
            std::vector<uint8_t> name;
            if (i < 3)
            {
                static constexpr const char* names[]{ "HP", "MP", "ST" };
                name.assign(names[i], names[i] + 2);
            }
            else
            {
                name = font.text_bytes(labels[i]);
            }
            unsigned color = 235;
            if (!i) { color = 234; }
            auto labelResult = font.try_draw_original_large(image, name, x + 51, row, color, 54);
            if (!labelResult) { return Result<std::vector<uint8_t>>::failure(labelResult.error()); }
            auto old = std::to_string(values[i]);
            auto oldResult = font.try_draw_original_large(image, { old.begin(), old.end() }, x + 136, row, 104);
            if (!oldResult) { return Result<std::vector<uint8_t>>::failure(oldResult.error()); }
            graphics.shared_panel(image, 0x4F72B0, x + 201, row + 7);
            auto next = std::to_string(values[i] + step.increments[i]);
            auto nextResult = font.try_draw_original_large(image, { next.begin(), next.end() }, x + 260, row, 36);
            if (!nextResult) { return Result<std::vector<uint8_t>>::failure(nextResult.error()); }
            row += 26;
        }
        return Result<std::vector<uint8_t>>::success(std::move(image));
    }
};
}    // namespace s2ne
