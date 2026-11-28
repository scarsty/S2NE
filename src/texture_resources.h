#pragma once
#include "framebuffer.h"
#include "resource_result.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
namespace s2ne
{
class TextureResources
{
    std::filesystem::path root_;
    SDL_Renderer* renderer_{};
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> target_{ nullptr, SDL_DestroyTexture };
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> background_{ nullptr, SDL_DestroyTexture };
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> presentation_{ nullptr, SDL_DestroyTexture };
    std::shared_ptr<TTF_Font> large_font_, small_font_;
    std::unordered_map<std::string, std::shared_ptr<TTF_Font>> fonts_;
    bool ttf_owned_{};
    std::unordered_map<std::string, std::shared_ptr<SDL_Texture>> textures_;
    std::unordered_map<std::string, bool> groups_;
    std::unordered_map<std::string, bool> available_ui_;
    std::unordered_map<std::string, std::shared_ptr<SDL_Texture>> text_textures_;
    std::unordered_map<std::string, std::shared_ptr<SDL_Surface>> text_surfaces_;
    std::unordered_map<std::string, bool> logged_;
    std::unordered_map<unsigned, std::array<std::pair<unsigned, unsigned>, 30>> sequences_;
    std::unordered_map<std::string, std::unordered_map<unsigned, std::vector<unsigned>>> cycles_;
    uint64_t elapsed_{};
    bool batching_{};
    bool map_available_{};
    SDL_Texture* batch_previous_{};
    std::array<uint8_t, 768> indexed_palette_{};
    std::unordered_map<uint32_t, uint8_t> palette_indices_;

    class TargetRestore
    {
        SDL_Renderer* renderer_;
        SDL_Texture* target_;
        bool active_ = true;

    public:
        explicit TargetRestore(SDL_Renderer* renderer);
        void release();
        ~TargetRestore();
    };

    struct Image
    {
        SharedBytes pixels;
        int width{}, height{};
    };
    std::unordered_map<std::string, Image> images_;

    static void copy_surface_pixels(const SDL_Surface& surface, std::span<uint8_t> pixels);

    static Result<Image> decode_image(const std::filesystem::path& path);

    Result<std::vector<char>> read_resource(const std::string& relative) const;

    static Result<SDL_Surface*> load_png(const std::filesystem::path& path);

public:
    Result<Image> load_image(const std::string& group, unsigned id);

    Result<Image> load_image_scaled(const std::string& group, unsigned id, int width, int height);

    static Result<Image> load_file_scaled(const std::filesystem::path& path, int width, int height);

private:
    static Result<Image> scale_image(Result<Image> loaded, int width, int height);

public:
    Result<bool> draw_small_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color);

private:
    Result<std::string> resolve_frame(const std::string& group, unsigned frame);
    Result<SDL_Texture*> load(const std::string& group, unsigned frame);

    Result<bool> readback(std::vector<uint8_t>& image);

    Result<bool> prepare_canvas(std::span<const uint8_t> image);

public:
    Result<bool> initialize_small_font(const std::filesystem::path& font_file);

private:
    Result<std::shared_ptr<TTF_Font>> load_font(const std::filesystem::path& path);

public:
    template <class Draw>
    Result<bool> draw_text_batch(std::vector<uint8_t>& image, Draw&& draw)
    {
        if (batching_) { return draw(image); }
        auto output = image;
        auto begun = begin_canvas(output);
        if (!begun) { return begun; }
        auto drawn = draw(output);
        auto ended = end_canvas(output);
        if (!drawn) { return drawn; }
        if (!ended) { return ended; }
        image.swap(output);
        return Result<bool>::success(true);
    }
    void set_elapsed(uint64_t elapsed);
    uint64_t elapsed() const;
    Result<bool> begin_canvas(std::span<const uint8_t> image);
    Result<bool> end_canvas(std::vector<uint8_t>& image);
    Result<bool> draw_cached(std::vector<uint8_t>& image, SDL_Texture* source, int left, int top);
    Result<bool> draw_glyph(std::vector<uint8_t>& image, std::span<const uint8_t> bits, unsigned width, unsigned height, int left, int top, std::span<const uint8_t> palette, unsigned color, std::vector<uint8_t>* indexed);
    Result<bool> tint(std::vector<uint8_t>& image, int left, int top, int width, int height, uint8_t red, uint8_t green, uint8_t blue, unsigned amount);
    void clear();
    void configure(const std::filesystem::path& working);
    bool active() const;
    Result<bool> group_available(const std::string& group);
    bool map_available() const;
    void bind(SDL_Renderer* renderer);
    Result<bool> initialize_font(const std::filesystem::path& font_file);
    bool font_ready() const;
    bool small_font_ready() const;
    Result<bool> draw_large_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color);
    Result<bool> draw_ttf_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color, TTF_Font* font, float size);
    Result<bool> draw_group(std::vector<uint8_t>& image, const std::string& group, unsigned id, int left, int top,
        std::span<const uint8_t> palette, std::vector<uint8_t>* indexed);
    Result<bool> draw_unit(std::vector<uint8_t>& image, unsigned appearance, unsigned action, unsigned direction, uint64_t elapsed, int left, int top, unsigned width, unsigned height, std::span<const uint8_t> mask);
    Result<bool> draw_masked(std::vector<uint8_t>& image, const std::string& group, unsigned frame, int left, int top, unsigned width, unsigned height, std::span<const uint8_t> mask);
    Result<bool> draw_ui(std::vector<uint8_t>& image, unsigned id, int left, int top,
        std::span<const uint8_t> palette, std::vector<uint8_t>* indexed);
    Result<bool> has_ui(unsigned id);
    Result<bool> draw_map_sprite(std::vector<uint8_t>& image, const std::string& group, unsigned id, int left, int top);
    Result<SDL_Texture*> upload_frame(std::span<const uint8_t> image);
};
inline TextureResources texture_resources;
}    //namespace s2ne
