#pragma once
#include "ZipFile2.h"
#include "resource_result.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
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
    std::unique_ptr<ZipFile2> archive_;
    SDL_Renderer* renderer_{};
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> target_{ nullptr, SDL_DestroyTexture };
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> background_{ nullptr, SDL_DestroyTexture };
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> presentation_{ nullptr, SDL_DestroyTexture };
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> large_font_{ nullptr, TTF_CloseFont };
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> small_font_{ nullptr, TTF_CloseFont };
    std::unordered_map<std::string, std::shared_ptr<SDL_Texture>> textures_;
    std::unordered_map<std::string, bool> logged_;
    std::unordered_map<unsigned, std::array<std::pair<unsigned, unsigned>, 30>> sequences_;
    std::unordered_map<std::string, std::unordered_map<unsigned, std::vector<unsigned>>> cycles_;
    struct TextCommand
    {
        std::string text;
        int left{}, top{};
        SDL_Color color{};
        bool small{};
    };
    std::vector<TextCommand> text_commands_;
    std::vector<TextCommand> presented_text_commands_;
    uint64_t elapsed_{};
    bool batching_{};
    SDL_Texture* batch_previous_{};

    class TargetRestore
    {
        SDL_Renderer* renderer_;
        SDL_Texture* target_;

    public:
        explicit TargetRestore(SDL_Renderer* renderer) : renderer_(renderer), target_(SDL_GetRenderTarget(renderer)) {}
        ~TargetRestore()
        {
            if (!SDL_SetRenderTarget(renderer_, target_)) { std::println(stderr, "Texture render target restore failed: {}", SDL_GetError()); }
        }
    };

    struct Image
    {
        std::vector<uint8_t> pixels;
        int width{}, height{};
    };

    Result<std::vector<char>> read_resource(const std::string& relative) const
    {
        if (archive_)
        {
            try
            {
                return Result<std::vector<char>>::success([&]
                    {
                        std::vector<char> content;
                        archive_->readFileToBuffer(relative, content);
                        return content;
                    }());
            } catch (const std::exception& error)
            {
                return Result<std::vector<char>>::failure(make_error("Cannot read texture archive " + relative + ": " + error.what()));
            }
        }
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
    Result<Image> load_image(const std::string& group, unsigned id) const
    {
        if (!active()) { return Result<Image>::failure(make_error("Texture resources are not active")); }
        const auto relative = group + "/" + std::to_string(id) + ".png";
        auto loaded = load_png(relative);
        if (!loaded) { return Result<Image>::failure(loaded.error()); }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> source(loaded.value(), SDL_DestroySurface);
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> rgba(SDL_ConvertSurface(source.get(), SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
        if (!rgba) { return Result<Image>::failure(make_error("Invalid external image: " + relative)); }
        std::vector<uint8_t> image(size_t(rgba->w) * rgba->h * 4);
        for (int y = 0; y < rgba->h; ++y) { std::memcpy(image.data() + size_t(y) * rgba->w * 4, static_cast<uint8_t*>(rgba->pixels) + y * rgba->pitch, size_t(rgba->w) * 4); }
        return Result<Image>::success({ std::move(image), rgba->w, rgba->h });
    }

    Result<bool> draw_small_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color)
    {
        if (!small_font_) { return Result<bool>::failure(make_error("SDL_ttf small font is not initialized")); }
        text_commands_.push_back({ std::string(text), left, top, color, true });
        return Result<bool>::success(true);
    }

private:
    Result<SDL_Texture*> load(const std::string& group, unsigned frame)
    {
        if (!renderer_) { return Result<SDL_Texture*>::failure(make_error("Texture renderer not bound")); }
        std::string filename = std::to_string(frame);
        if (group == "ui" || group == "ground" || group == "objects" || group == "scenery")
        {
            if (!cycles_.contains(group))
            {
                auto frame_data = read_resource(group + "/frames.txt");
                if (!frame_data) { return Result<SDL_Texture*>::failure(frame_data.error()); }
                std::istringstream input(std::string(frame_data.value().begin(), frame_data.value().end()));
                std::unordered_map<unsigned, std::vector<unsigned>> cycles;
                std::string line;
                while (std::getline(input, line))
                {
                    std::istringstream row(line);
                    unsigned id{}, delay{};
                    char colon{};
                    if (!(row >> id >> colon) || colon != ':') { return Result<SDL_Texture*>::failure(make_error("Invalid animation table " + group + "/frames.txt")); }
                    while (row >> delay)
                    {
                        cycles[id].push_back(delay);
                        row >> std::ws;
                        if (row.eof()) { break; }
                        char comma{};
                        if (!(row >> comma) || comma != ',') { return Result<SDL_Texture*>::failure(make_error("Invalid animation separator " + group + "/frames.txt")); }
                    }
                    if (cycles[id].empty()) { return Result<SDL_Texture*>::failure(make_error("Empty animation " + group + "/frames.txt")); }
                }
                if (input.bad()) { return Result<SDL_Texture*>::failure(make_error("Cannot finish reading " + group + "/frames.txt")); }
                cycles_.emplace(group, std::move(cycles));
            }
            if (auto cycle = cycles_.at(group).find(frame); cycle != cycles_.at(group).end())
            {
                const auto& delays = cycle->second;
                unsigned total = 0;
                for (const auto delay : delays) { total += delay; }
                unsigned position = total ? unsigned(elapsed_ % total) : 0;
                unsigned selected = 0;
                for (; selected + 1 < delays.size() && position >= delays[selected]; ++selected) { position -= delays[selected]; }
                filename += "_" + std::to_string(selected);
            }
        }
        const auto key = group + "/" + filename;
        if (auto found = textures_.find(key); found != textures_.end()) { return Result<SDL_Texture*>::success(found->second.get()); }
        auto loaded = load_png(group + "/" + filename + ".png");
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

public:
    Result<bool> initialize_small_font(const std::filesystem::path& font_file)
    {
        if (!renderer_ || !TTF_WasInit()) { return Result<bool>::failure(make_error("SDL_ttf is not initialized")); }
        small_font_.reset(TTF_OpenFont(font_file.string().c_str(), 24.0f));
        if (!small_font_) { return Result<bool>::failure(make_error(font_file.string() + ": " + SDL_GetError())); }
        return Result<bool>::success(true);
    }

public:
    void set_elapsed(uint64_t elapsed) { elapsed_ = elapsed; }
    uint64_t elapsed() const { return elapsed_; }
    Result<bool> begin_canvas(std::span<const uint8_t> image)
    {
        if (!renderer_ || batching_ || image.size() != 640u * 480 * 4) { return Result<bool>::failure(make_error("Invalid texture canvas begin")); }
        if (!target_) { target_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480)); }
        if (!background_) { background_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480)); }
        if (!target_ || !background_) { return Result<bool>::failure(make_error(SDL_GetError())); }
        batch_previous_ = SDL_GetRenderTarget(renderer_);
        if (!SDL_UpdateTexture(background_.get(), nullptr, image.data(), 640 * 4) || !SDL_SetRenderTarget(renderer_, target_.get()) || !SDL_RenderTexture(renderer_, background_.get(), nullptr, nullptr)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        batching_ = true;
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
        if (!target_)
        {
            target_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480));
            background_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480));
            if (!target_ || !background_) { return Result<bool>::failure(make_error(SDL_GetError())); }
        }
        if (!batching_ && !SDL_UpdateTexture(background_.get(), nullptr, image.data(), 640 * 4)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        TargetRestore restore(renderer_);
        if (!batching_ && (!SDL_SetRenderTarget(renderer_, target_.get()) || !SDL_RenderTexture(renderer_, background_.get(), nullptr, nullptr))) { return Result<bool>::failure(make_error(SDL_GetError())); }
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
        auto result = draw_cached(image, glyph, left, top);
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
        archive_.reset();
        target_.reset();
        background_.reset();
        presentation_.reset();
        large_font_.reset();
        small_font_.reset();
        clear_text_commands();
        if (TTF_WasInit()) { TTF_Quit(); }
        renderer_ = nullptr;
        logged_.clear();
        sequences_.clear();
        cycles_.clear();
        batching_ = false;
        batch_previous_ = nullptr;
        root_.clear();
    }
    void configure(const std::filesystem::path& working)
    {
        clear();
        const auto archive = working / "texture.zip";
        const auto path = working / "texture";
        if (std::filesystem::exists(archive) && std::filesystem::is_regular_file(archive))
        {
            archive_ = std::make_unique<ZipFile2>();
            archive_->openRead(archive.string());
            root_ = archive;
            std::println(stderr, "Texture resource archive: {}", archive.string());
            return;
        }
        if (!std::filesystem::exists(path)) { return; }
        if (!std::filesystem::is_directory(path)) { throw std::runtime_error("texture is not a directory"); }
        root_ = path;
        std::println(stderr, "Texture resource directory: {}", root_.string());
    }
    bool active() const { return !root_.empty(); }
    void bind(SDL_Renderer* renderer) { renderer_ = renderer; }
    void clear_text_commands()
    {
        text_commands_.clear();
        presented_text_commands_.clear();
    }
    Result<bool> initialize_font(const std::filesystem::path& font_file)
    {
        if (!renderer_ || !TTF_Init()) { return Result<bool>::failure(make_error(SDL_GetError())); }
        large_font_.reset(TTF_OpenFont(font_file.string().c_str(), 36.0f));
        if (!large_font_) { return Result<bool>::failure(make_error(font_file.string() + ": " + SDL_GetError())); }
        return Result<bool>::success(true);
    }
    bool font_ready() const { return large_font_ != nullptr; }
    bool small_font_ready() const { return small_font_ != nullptr; }
    void cover_text(int left, int top, int width, int height)
    {
        std::erase_if(text_commands_, [=](const TextCommand& command)
            {
                return command.left >= left && command.left < left + width && command.top >= top && command.top < top + height;
            });
    }
    Result<bool> draw_large_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color)
    {
        if (!large_font_) { return Result<bool>::failure(make_error("SDL_ttf font is not initialized")); }
        text_commands_.push_back({ std::string(text), left, top, color, false });
        return Result<bool>::success(true);
    }
    Result<bool> draw_large_text_utf8(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color)
    {
        return draw_large_text(image, text, left, top, color);
    }
    Result<bool> draw_group(std::vector<uint8_t>& image, const std::string& group, unsigned id, int left, int top,
        std::span<const uint8_t> palette, std::vector<uint8_t>* indexed)
    {
        if (!renderer_) { return Result<bool>::failure(make_error("Texture renderer not bound")); }
        auto loaded = load(group, id);
        if (!loaded) { return Result<bool>::failure(loaded.error()); }
        auto* texture = loaded.value();
        if (!target_)
        {
            target_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480));
            background_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480));
            if (!target_ || !background_) { return Result<bool>::failure(make_error(SDL_GetError())); }
        }
        SDL_Texture* previous = SDL_GetRenderTarget(renderer_);
        struct Restore
        {
            SDL_Renderer* renderer;
            SDL_Texture* previous;
            ~Restore()
            {
                if (!SDL_SetRenderTarget(renderer, previous)) { std::println(stderr, "Texture render target restore failed: {}", SDL_GetError()); }
            }
        } restore{ renderer_, previous };
        float width{}, height{};
        if (!SDL_GetTextureSize(texture, &width, &height)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        SDL_FRect rectangle{ float(left), float(top), width, height };
        if ((!batching_ && !SDL_UpdateTexture(background_.get(), nullptr, image.data(), 640 * 4)) || (!batching_ && !SDL_SetRenderTarget(renderer_, target_.get())) || (!batching_ && !SDL_RenderTexture(renderer_, background_.get(), nullptr, nullptr)) || !SDL_RenderTexture(renderer_, texture, nullptr, &rectangle)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        if (batching_) { return Result<bool>::success(true); }
        auto read = readback(image);
        if (!read) { return read; }
        if (!indexed) { return Result<bool>::success(true); }
        for (int y = std::max(0, top); y < std::min(480, top + int(height)); ++y)
        {
            for (int x = std::max(0, left); x < std::min(640, left + int(width)); ++x)
            {
                const size_t position = size_t(y) * 640 + x;
                const auto* pixel = image.data() + position * 4;
                if (indexed)
                {
                    bool found = false;
                    for (unsigned c = 0; c < 256; ++c)
                    {
                        if (unsigned(palette[c * 3]) * 255 / 63 == pixel[0] && unsigned(palette[c * 3 + 1]) * 255 / 63 == pixel[1] && unsigned(palette[c * 3 + 2]) * 255 / 63 == pixel[2])
                        {
                            indexed->at(position) = uint8_t(c);
                            found = true;
                            break;
                        }
                    }
                    if (!found) { return Result<bool>::failure(make_error("PNG UI color cannot enter legacy indexed effect")); }
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
        if (!target_)
        {
            target_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 640, 480));
            background_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480));
            if (!target_ || !background_) { return Result<bool>::failure(make_error(SDL_GetError())); }
        }
        TargetRestore restore(renderer_);
        if (!batching_ && (!SDL_UpdateTexture(background_.get(), nullptr, image.data(), 640 * 4) || !SDL_SetRenderTarget(renderer_, target_.get()) || !SDL_RenderTexture(renderer_, background_.get(), nullptr, nullptr))) { return Result<bool>::failure(make_error(SDL_GetError())); }
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
        presented_text_commands_ = std::move(text_commands_);
        text_commands_.clear();
        return Result<SDL_Texture*>::success(presentation_.get());
    }
    Result<bool> flush_text()
    {
        if (presented_text_commands_.empty()) { return Result<bool>::success(true); }
        int output_width{}, output_height{};
        if (!SDL_GetRenderOutputSize(renderer_, &output_width, &output_height) || output_width <= 0 || output_height <= 0)
        {
            presented_text_commands_.clear();
            return Result<bool>::failure(make_error(SDL_GetError()));
        }
        const float scale = std::min(float(output_width) / 640.0f, float(output_height) / 480.0f);
        if (!SDL_SetRenderLogicalPresentation(renderer_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED))
        {
            presented_text_commands_.clear();
            return Result<bool>::failure(make_error(SDL_GetError()));
        }
        struct Restore
        {
            SDL_Renderer* renderer;
            ~Restore() { SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX); }
        } restore{ renderer_ };
        for (const auto& command : presented_text_commands_)
        {
            auto* font = command.small ? small_font_.get() : large_font_.get();
            const float base_size = command.small ? 16.0f : 24.0f;
            const int render_size = std::max(1, int(std::lround(base_size * scale)));
            if (!TTF_SetFontSize(font, float(render_size)))
            {
                presented_text_commands_.clear();
                return Result<bool>::failure(make_error(SDL_GetError()));
            }
            const auto key = std::string(command.small ? "font-window-small/" : "font-window/") + std::to_string(render_size) + "/" + command.text;
            auto found = textures_.find(key);
            if (found == textures_.end())
            {
                const SDL_Color white{ 255, 255, 255, 255 };
                std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(TTF_RenderText_Blended(font, command.text.c_str(), command.text.size(), white), SDL_DestroySurface);
                if (!surface)
                {
                    presented_text_commands_.clear();
                    return Result<bool>::failure(make_error(SDL_GetError()));
                }
                std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer_, surface.get()), SDL_DestroyTexture);
                if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND))
                {
                    presented_text_commands_.clear();
                    return Result<bool>::failure(make_error(SDL_GetError()));
                }
                found = textures_.emplace(key, std::move(texture)).first;
            }
            if (!SDL_SetTextureColorMod(found->second.get(), command.color.r, command.color.g, command.color.b))
            {
                presented_text_commands_.clear();
                return Result<bool>::failure(make_error(SDL_GetError()));
            }
            float width{}, height{};
            if (!SDL_GetTextureSize(found->second.get(), &width, &height))
            {
                presented_text_commands_.clear();
                return Result<bool>::failure(make_error(SDL_GetError()));
            }
            SDL_FRect destination{ (float(output_width) - 640.0f * scale) * 0.5f + command.left * scale,
                (float(output_height) - 480.0f * scale) * 0.5f + command.top * scale, width, height };
            if (!SDL_RenderTexture(renderer_, found->second.get(), nullptr, &destination))
            {
                presented_text_commands_.clear();
                return Result<bool>::failure(make_error(SDL_GetError()));
            }
        }
        return Result<bool>::success(true);
    }
};
inline TextureResources texture_resources;
}    //namespace s2ne
