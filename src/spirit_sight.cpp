#include "spirit_sight.h"

namespace s2ne
{
auto SpiritSight::kind_string(unsigned directory, unsigned index) const -> std::vector<uint8_t>
{
    size_t p = dword(kinds_, directory);
    for (unsigned n = 0; n < index; ++n)
    {
        while (kinds_.at(p)) { ++p; }
        ++p;
    }
    return text(kinds_, p, kinds_.size() - p);
}

SpiritSight::SpiritSight(const std::filesystem::path& root) : kinds_(read_shared_resource(root / "KINDSP.BIN").value()), cel_(read_shared_resource(root / "SUPERSP.CEL").value()), pal_(read_shared_resource(root / "SUPERSP.PAL").value()) {}

auto SpiritSight::screenshot_indexed_pcx(std::span<const uint8_t> indices) const -> Result<std::vector<uint8_t>>
{
    if (indices.size() != 640u * 480) { return Result<std::vector<uint8_t>>::failure(make_error("47AED0 invalid indexed framebuffer extent")); }
    std::vector<uint8_t> bytes(128, 0);
    bytes[0] = 10;
    bytes[1] = 5;
    bytes[2] = 1;
    bytes[3] = 8;
    bytes[8] = 127;
    bytes[9] = 2;
    bytes[10] = 223;
    bytes[11] = 1;
    bytes[12] = bytes[14] = 100;
    bytes[65] = 1;
    bytes[66] = 128;
    bytes[67] = 2;
    bytes[68] = 1;
    for (unsigned y = 0; y < 480; ++y)
    {
        const auto row = indices.subspan(size_t(y) * 640, 640);
        for (unsigned x = 0; x < 640;)
        {
            unsigned count = 1;
            while (count < 63 && x + count < 640 && row[x + count] == row[x]) { ++count; }
            if (count > 1 || row[x] >= 192) { bytes.push_back(uint8_t(192 | count)); }
            bytes.push_back(row[x]);
            x += count;
        }
    }
    bytes.push_back(12);
    for (unsigned c = 0; c < 768; ++c) { bytes.push_back(uint8_t(unsigned(pal_.at(c)) * 255 / 63)); }
    return Result<std::vector<uint8_t>>::success(std::move(bytes));
}

auto SpiritSight::screenshot_pcx(const std::vector<uint8_t>& image) const -> Result<std::vector<uint8_t>>
{
    if (image.size() != 640u * 480 * 4) { return Result<std::vector<uint8_t>>::failure(make_error("47AED0 invalid framebuffer extent")); }
    std::map<uint32_t, uint8_t> colors;
    for (unsigned c = 0; c < 256; ++c)
    {
        uint32_t rgb = 0;
        for (unsigned component = 0; component < 3; ++component) { rgb |= (unsigned(pal_.at(c * 3 + component)) * 255 / 63) << (8 * component); }
        colors.try_emplace(rgb, uint8_t(c));
    }
    std::vector<uint8_t> indices(640 * 480);
    for (size_t p = 0; p < indices.size(); ++p)
    {
        const uint32_t rgb = unsigned(image[p * 4]) | unsigned(image[p * 4 + 1]) << 8 | unsigned(image[p * 4 + 2]) << 16;
        const auto color = colors.find(rgb);
        if (color == colors.end()) { return Result<std::vector<uint8_t>>::failure(make_error("47AED0 requires original palette pixels")); }
        indices[p] = color->second;
    }
    return screenshot_indexed_pcx(indices);
}

auto SpiritSight::save_screenshot(const std::filesystem::path& root, const std::vector<uint8_t>& image,
    std::optional<unsigned>& sequence, std::span<const uint8_t> indices) const -> Result<std::filesystem::path>
{
    auto encoded = indices.empty() ? screenshot_pcx(image) : screenshot_indexed_pcx(indices);
    if (!encoded) { return Result<std::filesystem::path>::failure(encoded.error()); }
    std::error_code ec;
    const auto directory = root / "SAVE";
    std::filesystem::create_directories(directory, ec);
    if (ec) { return Result<std::filesystem::path>::failure(make_error("4175C1 cannot create screenshot directory")); }
    unsigned next = sequence.value_or(0);
    if (!sequence)
    {
        for (std::filesystem::directory_iterator it(directory, ec), end; !ec && it != end; it.increment(ec))
        {
            const auto name = it->path().filename().string();
            int number = -1;
            if (name.size() >= 7 && SDL_strcasecmp(it->path().extension().string().c_str(), ".PCX") == 0 && sscanf(name.c_str(), "UJ%6d.PCX", &number) == 1 && number >= 0)
            {
                next = std::max(next, unsigned(number) + 1);
            }
        }
        if (ec) { return Result<std::filesystem::path>::failure(make_error("4175C1 cannot enumerate screenshots")); }
    }
    char name[32];
    snprintf(name, sizeof(name), "UJ%06u.PCX", next);
    const auto path = directory / name;
    if (std::filesystem::exists(path, ec) || ec) { return Result<std::filesystem::path>::failure(make_error("4175C1 screenshot destination already exists")); }
    std::ofstream file(path, std::ios::binary);
    const auto& bytes = encoded.value();
    file.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    file.close();
    if (!file) { return Result<std::filesystem::path>::failure(make_error("4175C1 cannot write screenshot")); }
    sequence = next + 1;
    return Result<std::filesystem::path>::success(path);
}

auto SpiritSight::draw(std::vector<uint8_t> image, const Menu& font, const Events& events,
    const Events::Unit& target, int screen_x, std::vector<uint8_t>* indexed) const -> Result<std::vector<uint8_t>>
{
    if (image.size() != 640u * 480 * 4 || (indexed && indexed->size() != 640u * 480)) { return Result<std::vector<uint8_t>>::failure(make_error("46864F invalid framebuffer extent")); }
    std::vector<uint8_t> indices;
    if (indexed) { indices = *indexed; }
    if (!target.appearance || target.appearance > word(kinds_, 22)) { return Result<std::vector<uint8_t>>::failure(make_error("46864F invalid target appearance")); }
    const size_t group = dword(pal_, 816) - 57;
    const size_t panel = group + dword(cel_, group + 2 + (239 - 142) * 8);
    const int width = int(word(cel_, panel)) + 1, height = int(word(cel_, panel + 2)) + 1;
    int x = screen_x + 32 - width / 2;
    if (x < 10) { x = 10; }
    else if (x + width > 639) { x = 630 - width; }
    const int y = 240 - height / 2;
    auto shadow = draw_original_panel_shadow(image, pal_, width, height, x, y, indexed ? &indices : nullptr);
    if (!shadow) { return Result<std::vector<uint8_t>>::failure(shadow.error()); }
    auto background = draw_original_cel(image, cel_, pal_, 239, x, y, ZeroRun::transparent, indexed ? &indices : nullptr);
    if (!background) { return Result<std::vector<uint8_t>>::failure(background.error()); }
    const size_t kind = dword(kinds_, 18) + size_t(target.appearance - 1) * 37;
    for (unsigned row = 0; row < 6; ++row)
    {
        auto label = font.try_draw_original_large(image, font.text_bytes(24 + row), x + 32, y + 32 + int(row) * 32, 108, 54, indexed ? &indices : nullptr);
        if (!label) { return Result<std::vector<uint8_t>>::failure(label.error()); }
    }
    const unsigned category_count = word(kinds_, 28);
    const size_t category_names = dword(kinds_, 24) + size_t(category_count) * category_count;
    auto category = text(kinds_, category_names + size_t(kinds_.at(kind + 17) - 1) * 9, 9);
    auto drawn = font.try_draw_original_large(image, category, x + 112, y + 32, 178, 54, indexed ? &indices : nullptr);
    if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
    for (unsigned row = 0; row < 2; ++row)
    {
        const unsigned mask = word(kinds_, kind + 22 + row * 2);
        int left = x + 112;
        unsigned shown = 0;
        if (!mask)
        {
            drawn = font.try_draw_original_large(image, font.text_bytes(30), left, y + 64 + int(row) * 32, 178, 54, indexed ? &indices : nullptr);
            if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
        }
        for (unsigned bit = 0; bit < word(kinds_, 16) && shown < 3; ++bit)
        {
            if (!(mask & (1u << bit))) { continue; }
            auto name = kind_string(12, bit);
            drawn = font.try_draw_original_large(image, name, left, y + 64 + int(row) * 32, 178, 54, indexed ? &indices : nullptr);
            if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
            left += int(name.size()) * 12 + 12;
            ++shown;
        }
    }
    auto movement = kind_string(6, kinds_.at(kind + 19));
    drawn = font.try_draw_original_large(image, movement, x + 112, y + 128, 178, 54, indexed ? &indices : nullptr);
    if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
    const auto value = events.try_combat_value(target);
    if (!value) { return Result<std::vector<uint8_t>>::failure(value.error()); }
    const auto number = std::to_string(value.value());
    drawn = font.try_draw_original_large(image, { number.begin(), number.end() }, x + 112, y + 160, 178, 54, indexed ? &indices : nullptr);
    if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
    auto actions = events.try_action_records(target);
    if (!actions) { return Result<std::vector<uint8_t>>::failure(actions.error()); }
    const size_t records = dword(kinds_, 30) + size_t(word(kinds_, 22)) * 4;
    for (size_t n = 0; n < actions.value().size(); ++n)
    {
        auto name = text(kinds_, records + size_t(actions.value()[n].index) * 23 + 1, 12);
        drawn = font.try_draw_original_large(image, name, x + 112 + int(n % 2) * 120, y + 192 + int(n / 2) * 32, 178, 54, indexed ? &indices : nullptr);
        if (!drawn) { return Result<std::vector<uint8_t>>::failure(drawn.error()); }
    }
    if (indexed) { indexed->swap(indices); }
    return Result<std::vector<uint8_t>>::success(std::move(image));
}
}    //namespace s2ne
