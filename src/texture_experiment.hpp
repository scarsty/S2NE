#pragma once
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace s2ne
{
inline int run_texture_experiment(const std::filesystem::path& root, unsigned visual, bool smoke)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { throw std::runtime_error(SDL_GetError()); }
    struct Quit
    {
        ~Quit() { SDL_Quit(); }
    } quit;
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("S2NE PNG texture experiment", 960, 720, 0), SDL_DestroyWindow);
    if (!window) { throw std::runtime_error(SDL_GetError()); }
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(SDL_CreateRenderer(window.get(), nullptr), SDL_DestroyRenderer);
    if (!renderer || !SDL_SetRenderLogicalPresentation(renderer.get(), 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)) { throw std::runtime_error(SDL_GetError()); }
    auto load = [&](const std::filesystem::path& path)
    {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(SDL_LoadPNG(path.string().c_str()), SDL_DestroySurface);
        if (!surface) { throw std::runtime_error(path.string() + ": " + SDL_GetError()); }
        std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture);
        if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND)) { throw std::runtime_error(SDL_GetError()); }
        return texture;
    };
    const auto folder = root / "spells" / std::to_string(visual);
    std::ifstream description(folder / "frames.txt");
    std::string label;
    unsigned count{}, delay{};
    int x{}, y{};
    char comma{};
    if (!(description >> label >> count) || label != "count:" || !count || !(description >> label >> delay) || label != "delay_ms:" || !delay || !(description >> label >> x >> comma >> y) || label != "anchor:" || comma != ',') { throw std::runtime_error("Invalid texture experiment frames.txt"); }
    std::vector<unsigned> durations;
    std::ifstream cycle_file(root / "battle-backdrops" / "frames.txt");
    if (!cycle_file) { throw std::runtime_error("Missing battlefield frames.txt"); }
    std::string line;
    while (std::getline(cycle_file, line))
    {
        std::istringstream row(line);
        unsigned id{}, duration{};
        char colon{};
        if (!(row >> id >> colon) || colon != ':') { throw std::runtime_error("Invalid battlefield frame row"); }
        if (id != 1) { continue; }
        while (row >> duration)
        {
            if (!duration) { throw std::runtime_error("Invalid battlefield frame duration"); }
            durations.push_back(duration);
            if (row.eof()) { break; }
            if (!(row >> comma) || comma != ',') { throw std::runtime_error("Invalid battlefield frame separator"); }
        }
        if (durations.empty() || !row.eof()) { throw std::runtime_error("Invalid battlefield frame durations"); }
    }
    uint64_t background_period = 0;
    for (auto duration : durations) { background_period += duration; }
    unsigned background_frame = 0;
    auto background = load(root / "battle-backdrops" / (durations.empty() ? "1.png" : "1_0.png"));
    auto effect = load(folder / "0.png");
    unsigned current = 0;
    bool running = true;
    const auto start = SDL_GetTicks();
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) { running = false; }
        }
        const auto elapsed = SDL_GetTicks() - start;
        if (background_period)
        {
            auto time = elapsed % background_period;
            unsigned selected = 0;
            while (time >= durations.at(selected)) { time -= durations.at(selected++); }
            if (selected != background_frame)
            {
                background = load(root / "battle-backdrops" / ("1_" + std::to_string(selected) + ".png"));
                background_frame = selected;
            }
        }
        const unsigned frame = unsigned(elapsed / delay % count);
        if (frame != current)
        {
            effect = load(folder / (std::to_string(frame) + ".png"));
            current = frame;
        }
        if (!SDL_SetRenderDrawColor(renderer.get(), 24, 24, 24, 255) || !SDL_RenderClear(renderer.get()) || !SDL_RenderTexture(renderer.get(), background.get(), nullptr, nullptr)) { throw std::runtime_error(SDL_GetError()); }
        float w{}, h{};
        if (!SDL_GetTextureSize(effect.get(), &w, &h)) { throw std::runtime_error(SDL_GetError()); }
        SDL_FRect destination{ 320.f - float(x), 400.f - float(y), w, h };
        if (x == -1) { destination.x = 0; }
        if (y == -1) { destination.y = 0; }
        if (!SDL_RenderTexture(renderer.get(), effect.get(), nullptr, &destination) || !SDL_RenderPresent(renderer.get())) { throw std::runtime_error(SDL_GetError()); }
        if (smoke && elapsed >= uint64_t(delay) * count) { running = false; }
        SDL_Delay(10);
    }
    return 0;
}
}    //namespace s2ne
