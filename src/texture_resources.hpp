#pragma once
#include "resource_result.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <array>
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
        explicit TargetRestore(SDL_Renderer* renderer) : renderer_(renderer), target_(SDL_GetRenderTarget(renderer)) {}
        void release() { active_ = false; }
        ~TargetRestore()
        {
            if (active_ && !SDL_SetRenderTarget(renderer_, target_)) { std::println(stderr, "Texture render target restore failed: {}", SDL_GetError()); }
        }
    };

    struct Image
    {
        SharedBytes pixels;
        int width{}, height{};
    };
    std::unordered_map<std::string, Image> images_;

    Result<std::vector<char>> read_resource(const std::string& relative) const
    {
        const auto file = root_ / std::filesystem::path(relative);
        std::ifstream input(file, std::ios::binary);
        if (!input) { return Result<std::vector<char>>::failure(make_error("Cannot read " + file.string())); }
        return Result<std::vector<char>>::success({ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() });
    }

    Result<SDL_Surface*> load_png(const std::string& relative) const
    {
        auto bytes = read_resource(relative);
        if (!bytes) { return Result<SDL_Surface*>::failure(bytes.error()); }
        auto buffer = std::move(bytes).value();
        auto* io = SDL_IOFromConstMem(buffer.data(), buffer.size());
        if (!io) { return Result<SDL_Surface*>::failure(make_error(SDL_GetError())); }
        auto* surface = SDL_LoadPNG_IO(io, true);
        if (!surface) { return Result<SDL_Surface*>::failure(make_error(relative + ": " + SDL_GetError())); }
        return Result<SDL_Surface*>::success(surface);
    }

public:
    Result<Image> load_image(const std::string& group, unsigned id)
    {
        if (!active()) { return Result<Image>::failure(make_error("Texture resources are not active")); }
        const auto relative = group + "/" + std::to_string(id) + ".png";
        if (auto cached = images_.find(relative); cached != images_.end()) { return Result<Image>::success(cached->second); }
        auto loaded = load_png(relative);
        if (!loaded) { return Result<Image>::failure(loaded.error()); }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> source(loaded.value(), SDL_DestroySurface);
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> rgba(SDL_ConvertSurface(source.get(), SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
        if (!rgba) { return Result<Image>::failure(make_error("Invalid external image: " + relative)); }
        std::vector<uint8_t> image(size_t(rgba->w) * rgba->h * 4);
        for (int y = 0; y < rgba->h; ++y) { std::memcpy(image.data() + size_t(y) * rgba->w * 4, static_cast<uint8_t*>(rgba->pixels) + y * rgba->pitch, size_t(rgba->w) * 4); }
        Image result{ SharedBytes(std::move(image)), rgba->w, rgba->h };
        images_.emplace(relative, result);
        return Result<Image>::success(std::move(result));
    }

    Result<bool> draw_small_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color)
    {
        if (!small_font_) { return Result<bool>::failure(make_error("SDL_ttf small font is not initialized")); }
        return draw_ttf_text(image, text, left, top, color, small_font_.get(), 16.0f);
    }

private:
    Result<std::string> resolve_frame(const std::string& group, unsigned frame)
    {
        std::string filename = std::to_string(frame);
        if (group == "ui" || group == "ground" || group == "objects" || group == "scenery")
        {
            if (!cycles_.contains(group))
            {
                auto frame_data = read_resource(group + "/frames.txt");
                if (!frame_data) { return Result<std::string>::failure(frame_data.error()); }
                std::istringstream input(std::string(frame_data.value().begin(), frame_data.value().end()));
                std::unordered_map<unsigned, std::vector<unsigned>> cycles;
                std::string line;
                while (std::getline(input, line))
                {
                    std::istringstream row(line);
                    unsigned id{}, delay{};
                    char colon{};
                    row >> std::ws;
                    if (row.peek() < '0' || row.peek() > '9') { return Result<std::string>::failure(make_error("Invalid animation identifier " + group + "/frames.txt")); }
                    if (!(row >> id >> colon) || colon != ':') { return Result<std::string>::failure(make_error("Invalid animation table " + group + "/frames.txt")); }
                    if (cycles.contains(id)) { return Result<std::string>::failure(make_error("Duplicate animation " + group + "/frames.txt")); }
                    for (;;)
                    {
                        row >> std::ws;
                        if (row.peek() < '0' || row.peek() > '9') { return Result<std::string>::failure(make_error("Invalid animation delay " + group + "/frames.txt")); }
                        if (!(row >> delay) || !delay) { return Result<std::string>::failure(make_error("Invalid animation delay " + group + "/frames.txt")); }
                        cycles[id].push_back(delay);
                        row >> std::ws;
                        if (row.eof()) { break; }
                        char comma{};
                        if (!(row >> comma) || comma != ',') { return Result<std::string>::failure(make_error("Invalid animation separator " + group + "/frames.txt")); }
                    }
                    if (cycles[id].empty()) { return Result<std::string>::failure(make_error("Empty animation " + group + "/frames.txt")); }
                }
                if (input.bad()) { return Result<std::string>::failure(make_error("Cannot finish reading " + group + "/frames.txt")); }
                cycles_.emplace(group, std::move(cycles));
            }
            if (auto cycle = cycles_.at(group).find(frame); cycle != cycles_.at(group).end())
            {
                const auto& delays = cycle->second;
                uint64_t total = 0;
                for (const auto delay : delays) { total += delay; }
                uint64_t position = elapsed_ % total;
                unsigned selected = 0;
                for (; selected + 1 < delays.size() && position >= delays[selected]; ++selected) { position -= delays[selected]; }
                filename += "_" + std::to_string(selected);
            }
        }
        return Result<std::string>::success(group + "/" + filename);
    }
    Result<SDL_Texture*> load(const std::string& group, unsigned frame)
    {
        if (!renderer_) { return Result<SDL_Texture*>::failure(make_error("Texture renderer not bound")); }
        auto resolved = resolve_frame(group, frame);
        if (!resolved) { return Result<SDL_Texture*>::failure(resolved.error()); }
        const auto key = std::move(resolved).value();
        if (auto found = textures_.find(key); found != textures_.end()) { return Result<SDL_Texture*>::success(found->second.get()); }
        auto loaded = load_png(key + ".png");
        if (!loaded) { return Result<SDL_Texture*>::failure(loaded.error()); }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(loaded.value(), SDL_DestroySurface);
        std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer_, surface.get()), SDL_DestroyTexture);
        if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND) || !SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST)) { return Result<SDL_Texture*>::failure(make_error(SDL_GetError())); }
        auto* result = texture.get();
        textures_.emplace(key, std::move(texture));
        if (!logged_.contains(group))
        {
            std::println(stderr, "Load texture group: {} (PNG, renderer-owned cache)", group);
            logged_[group] = true;
        }
        return Result<SDL_Texture*>::success(result);
    }

    Result<bool> readback(std::vector<uint8_t>& image)
    {
        if (image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("Invalid texture readback framebuffer")); }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> pixels(SDL_RenderReadPixels(renderer_, nullptr), SDL_DestroySurface);
        if (!pixels) { return Result<bool>::failure(make_error(SDL_GetError())); }
        if (pixels->format != SDL_PIXELFORMAT_RGBA32)
        {
            pixels.reset(SDL_ConvertSurface(pixels.get(), SDL_PIXELFORMAT_RGBA32));
            if (!pixels) { return Result<bool>::failure(make_error(SDL_GetError())); }
        }
        if (pixels->w != 640 || pixels->h != 480) { return Result<bool>::failure(make_error("Invalid texture readback extent")); }
        for (int y = 0; y < 480; ++y) { std::memcpy(image.data() + size_t(y) * 640 * 4, static_cast<uint8_t*>(pixels->pixels) + y * pixels->pitch, 640 * 4); }
        return Result<bool>::success(true);
    }

    Result<bool> prepare_canvas(std::span<const uint8_t> image)
    {
        if (!renderer_ || image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("Invalid texture canvas framebuffer")); }
        if (batching_) { return Result<bool>::success(true); }
        if (!target_) { target_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480)); }
        if (!background_) { background_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480)); }
        if (!target_ || !background_) { return Result<bool>::failure(make_error(SDL_GetError())); }
        if (!SDL_SetTextureBlendMode(background_.get(), SDL_BLENDMODE_NONE) || !SDL_SetTextureBlendMode(target_.get(), SDL_BLENDMODE_NONE)
            || !SDL_UpdateTexture(background_.get(), nullptr, image.data(), 640 * 4) || !SDL_SetRenderTarget(renderer_, target_.get())
            || !SDL_RenderTexture(renderer_, background_.get(), nullptr, nullptr)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        return Result<bool>::success(true);
    }

public:
    Result<bool> initialize_small_font(const std::filesystem::path& font_file)
    {
        auto loaded = load_font(font_file);
        if (!loaded) { return Result<bool>::failure(loaded.error()); }
        small_font_ = std::move(loaded).value();
        return Result<bool>::success(true);
    }

private:
    Result<std::shared_ptr<TTF_Font>> load_font(const std::filesystem::path& path)
    {
        if (!renderer_) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error("Texture renderer not bound")); }
        std::error_code error;
        auto canonical = std::filesystem::weakly_canonical(path, error);
        if (error) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error(error.message())); }
#ifdef _WIN32
        auto native = canonical.native();
        for (auto& character : native) { character = wchar_t(std::towlower(character)); }
        canonical = std::filesystem::path(native);
#endif
        const auto key = canonical.string();
        if (auto found = fonts_.find(key); found != fonts_.end()) { return Result<std::shared_ptr<TTF_Font>>::success(found->second); }
        if (!ttf_owned_)
        {
            if (!TTF_Init()) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error(SDL_GetError())); }
            ttf_owned_ = true;
        }
        std::shared_ptr<TTF_Font> font(TTF_OpenFont(path.string().c_str(), 24.0f), TTF_CloseFont);
        if (!font) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error(path.string() + ": " + SDL_GetError())); }
        fonts_.emplace(key, font);
        return Result<std::shared_ptr<TTF_Font>>::success(std::move(font));
    }

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
    void set_elapsed(uint64_t elapsed) { elapsed_ = elapsed; }
    uint64_t elapsed() const { return elapsed_; }
    Result<bool> begin_canvas(std::span<const uint8_t> image)
    {
        if (!renderer_ || batching_ || image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("Invalid texture canvas begin")); }
        batch_previous_ = SDL_GetRenderTarget(renderer_);
        TargetRestore restore(renderer_);
        auto prepared = prepare_canvas(image);
        if (!prepared) { return prepared; }
        batching_ = true;
        restore.release();
        return Result<bool>::success(true);
    }
    Result<bool> end_canvas(std::vector<uint8_t>& image)
    {
        if (!batching_) { return Result<bool>::failure(make_error("Texture canvas not active")); }
        auto result = readback(image);
        batching_ = false;
        if (!SDL_SetRenderTarget(renderer_, batch_previous_)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        return result;
    }
    Result<bool> draw_cached(std::vector<uint8_t>& image, SDL_Texture* source, int left, int top)
    {
        if (!renderer_ || !source || image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("Invalid cached texture draw")); }
        TargetRestore restore(renderer_);
        auto prepared = prepare_canvas(image);
        if (!prepared) { return prepared; }
        float width{}, height{};
        if (!SDL_GetTextureSize(source, &width, &height)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        SDL_FRect rectangle{ float(left), float(top), width, height };
        if (!SDL_RenderTexture(renderer_, source, nullptr, &rectangle)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        if (batching_) { return Result<bool>::success(true); }
        return readback(image);
    }
    Result<bool> draw_glyph(std::vector<uint8_t>& image, std::span<const uint8_t> bits, unsigned width, unsigned height, int left, int top, std::span<const uint8_t> palette, unsigned color, std::vector<uint8_t>* indexed)
    {
        if (!renderer_ || !width || width % 8 || bits.size() != size_t(width / 8) * height || palette.size() < 768 || color > 255) { return Result<bool>::failure(make_error("Invalid original glyph texture")); }
        std::string key = "glyph/" + std::to_string(width) + "/" + std::to_string(height) + "/";
        key.append(reinterpret_cast<const char*>(bits.data()), bits.size());
        if (!textures_.contains(key))
        {
            std::vector<uint8_t> rgba(size_t(width) * height * 4, 255);
            for (unsigned y = 0; y < height; ++y)
            {
                for (unsigned x = 0; x < width; ++x) { rgba[(size_t(y) * width + x) * 4 + 3] = (bits[size_t(y) * (width / 8) + x / 8] & (128 >> (x % 8))) ? 255 : 0; }
            }
            std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(SDL_CreateSurfaceFrom(int(width), int(height), SDL_PIXELFORMAT_RGBA32, rgba.data(), int(width) * 4), SDL_DestroySurface);
            if (!surface) { return Result<bool>::failure(make_error(SDL_GetError())); }
            std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer_, surface.get()), SDL_DestroyTexture);
            if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND)) { return Result<bool>::failure(make_error(SDL_GetError())); }
            textures_.emplace(key, std::move(texture));
            if (!logged_.contains("fonts"))
            {
                std::println(stderr, "Load texture group: fonts (original glyphs, renderer-owned cache)");
                logged_["fonts"] = true;
            }
        }
        auto* glyph = textures_.at(key).get();
        if (!SDL_SetTextureColorMod(glyph, uint8_t(unsigned(palette[color * 3]) * 255 / 63), uint8_t(unsigned(palette[color * 3 + 1]) * 255 / 63), uint8_t(unsigned(palette[color * 3 + 2]) * 255 / 63))) { return Result<bool>::failure(make_error(SDL_GetError())); }
        auto result = Result<bool>::success(true);
        if (batching_) { result = draw_cached(image, glyph, left, top); }
        else
        {
            if (image.size() != 640u * 480 * 4 || (indexed && indexed->size() != 640u * 480)) { return Result<bool>::failure(make_error("Invalid glyph framebuffer")); }
            for (unsigned y = 0; y < height; ++y)
            {
                for (unsigned x = 0; x < width; ++x)
                {
                    const int dx = left + int(x), dy = top + int(y);
                    if (dx < 0 || dx >= 640 || dy < 0 || dy >= 480 || !(bits[size_t(y) * (width / 8) + x / 8] & (128 >> (x % 8)))) { continue; }
                    const size_t p = (size_t(dy) * 640 + dx) * 4;
                    for (unsigned k = 0; k < 3; ++k) { image[p + k] = uint8_t(unsigned(palette[color * 3 + k]) * 255 / 63); }
                    image[p + 3] = 255;
                }
            }
        }
        if (!result || !indexed) { return result; }
        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width; ++x)
            {
                const int dx = left + int(x), dy = top + int(y);
                if (dx >= 0 && dx < 640 && dy >= 0 && dy < 480 && (bits[size_t(y) * (width / 8) + x / 8] & (128 >> (x % 8)))) { indexed->at(size_t(dy) * 640 + dx) = uint8_t(color); }
            }
        }
        return result;
    }
    Result<bool> tint(std::vector<uint8_t>& image, int left, int top, int width, int height, uint8_t red, uint8_t green, uint8_t blue, unsigned amount)
    {
        if (amount > 100) { return Result<bool>::failure(make_error("Invalid texture tint amount")); }
        auto begun = begin_canvas(image);
        if (!begun) { return begun; }
        SDL_FRect rectangle{ float(left), float(top), float(width), float(height) };
        const bool drawn = SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND) && SDL_SetRenderDrawColor(renderer_, red, green, blue, uint8_t(amount * 255 / 100)) && SDL_RenderFillRect(renderer_, &rectangle);
        const std::string error = drawn ? "" : SDL_GetError();
        auto ended = end_canvas(image);
        if (!drawn) { return Result<bool>::failure(make_error(error)); }
        return ended;
    }
    void clear()
    {
        textures_.clear();
        text_textures_.clear();
        text_surfaces_.clear();
        images_.clear();
        groups_.clear();
        available_ui_.clear();
        target_.reset();
        background_.reset();
        presentation_.reset();
        large_font_.reset();
        small_font_.reset();
        fonts_.clear();
        if (ttf_owned_) { TTF_Quit(); }
        ttf_owned_ = false;
        renderer_ = nullptr;
        logged_.clear();
        sequences_.clear();
        cycles_.clear();
        palette_indices_.clear();
        batching_ = false;
        batch_previous_ = nullptr;
        map_available_ = false;
        root_.clear();
    }
    void configure(const std::filesystem::path& working)
    {
        clear();
        const auto path = working / "texture";
        if (!std::filesystem::exists(path)) { return; }
        if (!std::filesystem::is_directory(path)) { throw std::runtime_error("texture is not a directory"); }
        root_ = path;
        map_available_ = true;
        for (const auto* group : { "ground", "objects", "scenery" })
        {
            auto available = group_available(group);
            if (!available) { throw std::runtime_error(available.error().message); }
            map_available_ = map_available_ && available.value();
        }
        if (!map_available_) { std::println(stderr, "External map groups incomplete; retaining original map renderer"); }
        std::println(stderr, "Texture resource directory: {}", root_.string());
    }
    bool active() const { return !root_.empty(); }
    Result<bool> group_available(const std::string& group)
    {
        if (!active()) { return Result<bool>::success(false); }
        if (auto found = groups_.find(group); found != groups_.end()) { return Result<bool>::success(found->second); }
        std::error_code error;
        const auto path = root_ / group;
        const bool present = std::filesystem::exists(path, error);
        if (error) { return Result<bool>::failure(make_error(path.string() + ": " + error.message())); }
        if (present && !std::filesystem::is_directory(path, error)) { return Result<bool>::failure(make_error("Invalid texture group directory: " + path.string())); }
        if (error) { return Result<bool>::failure(make_error(path.string() + ": " + error.message())); }
        groups_.emplace(group, present);
        return Result<bool>::success(present);
    }
    bool map_available() const { return active() && map_available_; }
    void bind(SDL_Renderer* renderer) { renderer_ = renderer; }
    Result<bool> initialize_font(const std::filesystem::path& font_file)
    {
        auto loaded = load_font(font_file);
        if (!loaded) { return Result<bool>::failure(loaded.error()); }
        large_font_ = std::move(loaded).value();
        return Result<bool>::success(true);
    }
    bool font_ready() const { return large_font_ != nullptr; }
    bool small_font_ready() const { return small_font_ != nullptr; }
    Result<bool> draw_large_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color)
    {
        if (!large_font_) { return Result<bool>::failure(make_error("SDL_ttf font is not initialized")); }
        return draw_ttf_text(image, text, left, top, color, large_font_.get(), 24.0f);
    }
    Result<bool> draw_ttf_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color, TTF_Font* font, float size)
    {
        if (!font || image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("Invalid SDL_ttf draw target")); }
        const auto key = std::to_string(reinterpret_cast<uintptr_t>(font)) + "/" + std::to_string(size) + "/" + std::string(text);
        auto cached = text_textures_.find(key);
        if (cached == text_textures_.end())
        {
            if (!TTF_SetFontSize(font, size)) { return Result<bool>::failure(make_error(SDL_GetError())); }
            std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(TTF_RenderText_Blended(font, std::string(text).c_str(), text.size(), SDL_Color{ 255, 255, 255, 255 }), SDL_DestroySurface);
            if (!surface) { return Result<bool>::failure(make_error(SDL_GetError())); }
            std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer_, surface.get()), SDL_DestroyTexture);
            if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND)) { return Result<bool>::failure(make_error(SDL_GetError())); }
            text_surfaces_.emplace(key, std::shared_ptr<SDL_Surface>(surface.release(), SDL_DestroySurface));
            cached = text_textures_.emplace(key, std::move(texture)).first;
        }
        if (!batching_)
        {
            auto* surface = text_surfaces_.at(key).get();
            if (!SDL_SetSurfaceColorMod(surface, color.r, color.g, color.b) || !SDL_SetSurfaceAlphaMod(surface, color.a) || !SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND)) { return Result<bool>::failure(make_error(SDL_GetError())); }
            std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> destination(SDL_CreateSurfaceFrom(640, 480, SDL_PIXELFORMAT_RGBA32, image.data(), 640 * 4), SDL_DestroySurface);
            if (!destination) { return Result<bool>::failure(make_error(SDL_GetError())); }
            SDL_Rect rectangle{ left, top, surface->w, surface->h };
            if (!SDL_BlitSurface(surface, nullptr, destination.get(), &rectangle)) { return Result<bool>::failure(make_error(SDL_GetError())); }
            return Result<bool>::success(true);
        }
        auto* texture = cached->second.get();
        if (!SDL_SetTextureColorMod(texture, color.r, color.g, color.b) || !SDL_SetTextureAlphaMod(texture, color.a)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        return draw_cached(image, texture, left, top);
    }
    Result<bool> draw_group(std::vector<uint8_t>& image, const std::string& group, unsigned id, int left, int top,
        std::span<const uint8_t> palette, std::vector<uint8_t>* indexed)
    {
        if (!renderer_) { return Result<bool>::failure(make_error("Texture renderer not bound")); }
        if (image.size() != 640u * 480 * 4 || (indexed && (palette.size() < 768 || indexed->size() != 640u * 480))) { return Result<bool>::failure(make_error("Invalid PNG palette/framebuffer")); }
        auto loaded = load(group, id);
        if (!loaded) { return Result<bool>::failure(loaded.error()); }
        auto* texture = loaded.value();
        TargetRestore restore(renderer_);
        auto prepared = prepare_canvas(image);
        if (!prepared) { return prepared; }
        float width{}, height{};
        if (!SDL_GetTextureSize(texture, &width, &height)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        SDL_FRect rectangle{ float(left), float(top), width, height };
        if (!SDL_RenderTexture(renderer_, texture, nullptr, &rectangle)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        if (batching_) { return Result<bool>::success(true); }
        auto read = readback(image);
        if (!read) { return read; }
        if (!indexed) { return Result<bool>::success(true); }
        if (palette_indices_.empty() || !std::equal(indexed_palette_.begin(), indexed_palette_.end(), palette.begin()))
        {
            std::copy_n(palette.begin(), 768, indexed_palette_.begin());
            palette_indices_.clear();
            for (unsigned c = 0; c < 256; ++c)
            {
                uint32_t rgb = 0;
                for (unsigned k = 0; k < 3; ++k) { rgb |= (unsigned(palette[c * 3 + k]) * 255 / 63) << (k * 8); }
                palette_indices_.emplace(rgb, uint8_t(c));
            }
        }
        for (int y = std::max(0, top); y < std::min(480, top + int(height)); ++y)
        {
            for (int x = std::max(0, left); x < std::min(640, left + int(width)); ++x)
            {
                const size_t position = size_t(y) * 640 + x;
                const auto* pixel = image.data() + position * 4;
                if (indexed)
                {
                    uint32_t rgb = pixel[0] | uint32_t(pixel[1]) << 8 | uint32_t(pixel[2]) << 16;
                    auto found = palette_indices_.find(rgb);
                    if (found == palette_indices_.end()) { return Result<bool>::failure(make_error("PNG UI color cannot enter legacy indexed effect")); }
                    indexed->at(position) = found->second;
                }
            }
        }
        return Result<bool>::success(true);
    }
    Result<bool> draw_unit(std::vector<uint8_t>& image, unsigned appearance, unsigned action, unsigned direction, uint64_t elapsed, int left, int top, unsigned width, unsigned height, std::span<const uint8_t> mask)
    {
        if (action * 4 + direction >= 30 || mask.size() != size_t(width) * height) { return Result<bool>::failure(make_error("Invalid external unit sequence/mask")); }
        if (!sequences_.contains(appearance))
        {
            const auto relative = "units/" + std::to_string(appearance) + "/frames.txt";
            auto description = read_resource(relative);
            if (!description) { return Result<bool>::failure(description.error()); }
            std::istringstream input(std::string(description.value().begin(), description.value().end()));
            std::array<std::pair<unsigned, unsigned>, 30> sequences{};
            std::string line;
            while (std::getline(input, line))
            {
                std::istringstream row(line);
                unsigned a{}, d{}, first{}, count{};
                char comma{}, colon{}, second_comma{};
                if (!(row >> a >> comma >> d >> colon >> first >> second_comma >> count) || comma != ',' || colon != ':' || second_comma != ',' || a >= 8 || d >= 4 || a * 4 + d >= sequences.size() || sequences[a * 4 + d].second) { return Result<bool>::failure(make_error("Invalid unit frames: " + relative)); }
                row >> std::ws;
                if (!row.eof()) { return Result<bool>::failure(make_error("Unexpected unit frame data: " + relative)); }
                sequences[a * 4 + d] = { first, count };
            }
            if (input.bad()) { return Result<bool>::failure(make_error("Cannot finish reading " + relative)); }
            sequences_.emplace(appearance, sequences);
        }
        const auto [first, count] = sequences_.at(appearance)[action * 4 + direction];
        if (!count) { return Result<bool>::failure(make_error("Missing external unit sequence")); }
        return draw_masked(image, "units/" + std::to_string(appearance), first + unsigned((elapsed / 190) % count), left, top, width, height, mask);
    }
    Result<bool> draw_masked(std::vector<uint8_t>& image, const std::string& group, unsigned frame, int left, int top, unsigned width, unsigned height, std::span<const uint8_t> mask)
    {
        if (!width || !height || image.size() != 640u * 480 * 4 || mask.size() != size_t(width) * height) { return Result<bool>::failure(make_error("Invalid external texture mask")); }
        auto texture = load(group, frame);
        if (!texture) { return Result<bool>::failure(texture.error()); }
        TargetRestore restore(renderer_);
        auto prepared = prepare_canvas(image);
        if (!prepared) { return prepared; }
        float texture_width{}, texture_height{};
        if (!SDL_GetTextureSize(texture.value(), &texture_width, &texture_height)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width;)
            {
                if (!mask[size_t(y) * width + x])
                {
                    ++x;
                    continue;
                }
                unsigned start = x++;
                while (x < width && mask[size_t(y) * width + x]) { ++x; }
                SDL_FRect source{ float(start) * texture_width / width, float(y) * texture_height / height, float(x - start) * texture_width / width, texture_height / height };
                SDL_FRect destination{ float(left) + start, float(top) + y, float(x - start), 1 };
                if (!SDL_RenderTexture(renderer_, texture.value(), &source, &destination)) { return Result<bool>::failure(make_error(SDL_GetError())); }
            }
        }
        if (batching_) { return Result<bool>::success(true); }
        return readback(image);
    }
    Result<bool> draw_ui(std::vector<uint8_t>& image, unsigned id, int left, int top,
        std::span<const uint8_t> palette, std::vector<uint8_t>* indexed)
    {
        return draw_group(image, "ui", id, left, top, palette, indexed);
    }
    Result<bool> has_ui(unsigned id)
    {
        auto present = group_available("ui");
        if (!present || !present.value()) { return present; }
        auto resolved = resolve_frame("ui", id);
        if (!resolved) { return Result<bool>::failure(resolved.error()); }
        if (auto found = available_ui_.find(resolved.value()); found != available_ui_.end()) { return Result<bool>::success(found->second); }
        std::error_code error;
        bool exists = std::filesystem::exists(root_ / (resolved.value() + ".png"), error);
        if (error) { return Result<bool>::failure(make_error(error.message())); }
        if (!exists && cycles_.at("ui").contains(id)) { return Result<bool>::failure(make_error("Missing declared animation frame: " + resolved.value())); }
        available_ui_.emplace(resolved.value(), exists);
        return Result<bool>::success(exists);
    }
    Result<bool> draw_map_sprite(std::vector<uint8_t>& image, const std::string& group, unsigned id, int left, int top)
    {
        return draw_group(image, group, id, left, top, {}, nullptr);
    }
    Result<SDL_Texture*> upload_frame(std::span<const uint8_t> image)
    {
        if (!renderer_ || image.size() != 640u * 480 * 4) { return Result<SDL_Texture*>::failure(make_error("Invalid external frame upload")); }
        if (!presentation_)
        {
            presentation_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480));
            if (!presentation_ || !SDL_SetTextureScaleMode(presentation_.get(), SDL_SCALEMODE_LINEAR)) { return Result<SDL_Texture*>::failure(make_error(SDL_GetError())); }
        }
        if (!SDL_UpdateTexture(presentation_.get(), nullptr, image.data(), 640 * 4))
        {
            return Result<SDL_Texture*>::failure(make_error(SDL_GetError()));
        }
        return Result<SDL_Texture*>::success(presentation_.get());
    }
};
inline TextureResources texture_resources;
}    //namespace s2ne
