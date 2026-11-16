#pragma once
#include "resource_result.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
namespace s2ne
{
class TextureResources
{
    std::filesystem::path root_;
    std::unordered_map<unsigned, std::shared_ptr<SDL_Surface>> ui_;
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> target_{ nullptr, SDL_DestroySurface };
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer_{ nullptr, SDL_DestroyRenderer };
    std::unordered_map<std::string, std::shared_ptr<SDL_Texture>> textures_;

public:
    void clear()
    {
        textures_.clear();
        renderer_.reset();
        target_.reset();
        ui_.clear();
        root_.clear();
    }
    void configure(const std::filesystem::path& working)
    {
        clear();
        const auto path = working / "texture";
        if (!std::filesystem::exists(path)) { return; }
        if (!std::filesystem::is_directory(path)) { throw std::runtime_error("texture is not a directory"); }
        root_ = path;
        std::fprintf(stderr, "Texture resource directory: %s\n", root_.string().c_str());
    }
    bool active() const { return !root_.empty(); }
    Result<bool> draw_ui(std::vector<uint8_t>& image, unsigned id, int left, int top,
        std::span<const uint8_t> palette, std::vector<uint8_t>* indexed)
    {
        if (!ui_.contains(id))
        {
            const auto file = root_ / "ui" / (std::to_string(id) + ".png");
            auto* loaded = SDL_LoadPNG(file.string().c_str());
            if (!loaded) { return Result<bool>::failure(make_error(file.string() + ": " + SDL_GetError())); }
            std::shared_ptr<SDL_Surface> source(loaded, SDL_DestroySurface);
            auto* converted = SDL_ConvertSurface(source.get(), SDL_PIXELFORMAT_RGBA32);
            if (!converted) { return Result<bool>::failure(make_error(SDL_GetError())); }
            ui_.emplace(id, std::shared_ptr<SDL_Surface>(converted, SDL_DestroySurface));
            if (ui_.size() == 1) { std::fprintf(stderr, "Load texture group: ui (PNG, lazy cached)\n"); }
        }
        const auto& source = ui_.at(id);
        if (!renderer_)
        {
            target_.reset(SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_RGBA32));
            if (!target_) { return Result<bool>::failure(make_error(SDL_GetError())); }
            renderer_.reset(SDL_CreateSoftwareRenderer(target_.get()));
            if (!renderer_) { return Result<bool>::failure(make_error(SDL_GetError())); }
        }
        const auto key = "ui/" + std::to_string(id);
        if (!textures_.contains(key))
        {
            auto* texture = SDL_CreateTextureFromSurface(renderer_.get(), source.get());
            if (!texture) { return Result<bool>::failure(make_error(SDL_GetError())); }
            std::shared_ptr<SDL_Texture> owned(texture, SDL_DestroyTexture);
            if (!SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND)) { return Result<bool>::failure(make_error(SDL_GetError())); }
            textures_.emplace(key, std::move(owned));
        }
        for (int y = 0; y < 480; ++y) { std::memcpy(static_cast<uint8_t*>(target_->pixels) + y * target_->pitch, image.data() + size_t(y) * 640 * 4, 640 * 4); }
        SDL_FRect rectangle{ float(left), float(top), float(source->w), float(source->h) };
        if (!SDL_RenderTexture(renderer_.get(), textures_.at(key).get(), nullptr, &rectangle) || !SDL_FlushRenderer(renderer_.get())) { return Result<bool>::failure(make_error(SDL_GetError())); }
        for (int y = 0; y < 480; ++y) { std::memcpy(image.data() + size_t(y) * 640 * 4, static_cast<uint8_t*>(target_->pixels) + y * target_->pitch, 640 * 4); }
        if (!indexed) { return Result<bool>::success(true); }
        for (int y = 0; y < source->h; ++y)
        {
            const auto* pixels = static_cast<const uint8_t*>(source->pixels) + y * source->pitch;
            for (int x = 0; x < source->w; ++x)
            {
                const int dx = left + x, dy = top + y;
                const auto* pixel = pixels + x * 4;
                if (!pixel[3] || dx < 0 || dy < 0 || dx >= 640 || dy >= 480) { continue; }
                const size_t position = size_t(dy) * 640 + dx;
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
};
inline TextureResources texture_resources;
}    //namespace s2ne
