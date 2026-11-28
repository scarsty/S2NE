#include "title_image.h"

namespace s2ne
{
auto try_load_title(const std::filesystem::path& root, unsigned resource) -> Result<std::vector<uint8_t>>
{
    const auto external = root / "texture" / "opening" / (std::to_string(resource == 20 ? 19 : resource) + ".png");
    static std::filesystem::path cached_path;
    static int cached_width{}, cached_height{};
    static std::vector<uint8_t> cached_image;
    {
        if (cached_image.empty() || cached_path != external || cached_width != screen_width || cached_height != screen_height)
        {
            auto loaded = TextureResources::load_file_scaled(external, screen_width, screen_height);
            if (!loaded) { return Result<std::vector<uint8_t>>::failure(loaded.error()); }
            cached_image = loaded.value().pixels;
            cached_path = external;
            cached_width = screen_width;
            cached_height = screen_height;
        }
        std::vector<uint8_t> image = cached_image;
        if (resource == 20)
        {
            for (size_t p = 0; p < image.size(); p += 4)
            {
                for (unsigned c = 0; c < 3; ++c) { image[p + c] = uint8_t(unsigned(image[p + c]) * 30 / 100); }
            }
        }
        set_fixed_page_background(image);
        return Result<std::vector<uint8_t>>::success(fixed_page_center());
    }
}

auto load_title(const std::filesystem::path& root, unsigned resource) -> std::vector<uint8_t>
{
    auto result = try_load_title(root, resource);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}
}    //namespace s2ne
