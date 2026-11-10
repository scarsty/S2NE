#pragma once
#include "original_save_bridge.hpp"
#include "prologue.hpp"
#include "runtime_error.hpp"
namespace s2ne
{
inline void original_teleport_smoke(const std::filesystem::path& root, SDL_Renderer* renderer, SDL_Texture* texture)
{
    std::ifstream input(root / "ITEMSP.BIN", std::ios::binary);
    std::vector<uint8_t> items(std::istreambuf_iterator<char>(input), { });
    auto count = OriginalSave::item_count(items);
    std::optional<OriginalSave::State> raw;
    std::filesystem::path source;
    for (const auto& path : std::filesystem::recursive_directory_iterator(root / "S2NE" / "out"))
    {
        if (path.path().filename().wstring().find(L"stage60-start.sav") == std::wstring::npos)
        {
            continue;
        }
        auto loaded = OriginalSave::try_load(path.path(), count);
        if (loaded)
        {
            auto candidate = std::move(loaded).value();
            unsigned stage = unsigned(candidate.globals_4f7cd0[8]) | (unsigned(candidate.globals_4f7cd0[9]) << 8);
            if (stage == 60)
            {
                raw = std::move(candidate);
                source = path.path();
                break;
            }
        }
    }
    if (!raw)
    {
        s2ne::fatal("Teleport SDL needs preserved stage60 source save");
    }
    auto restored = OriginalSaveBridge::restore(root, *raw);
    auto pads = Battle::load(root, restored.events.map_id).teleport_pads();
    if (pads.size() < 2)
    {
        s2ne::fatal("Original stage60 pads");
    }
    unsigned actor = 0;
    while (actor < restored.events.units.size() && !restored.events.units[actor].controllable())
    {
        ++actor;
    }
    if (actor == restored.events.units.size())
    {
        s2ne::fatal("Original controllable role");
    }
    auto& role = restored.events.units[actor];
    role.x = pads[0].x;
    role.y = pads[0].y;
    role.status_flags = 0x80;
    role.action_state = 2;
    for (auto& u : restored.events.units)
    {
        if (u.controllable())
        {
            u.action_state = 2;
        }
    }
    auto before = OriginalSaveBridge::capture(restored.events, restored.turn, restored.camera, &*raw);
    auto write = [&](const char* name, const auto& save)
    {
        auto bytes = OriginalSave::encode(save, count);
        std::ofstream out(root / "S2NE" / "out" / name, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        if (!out)
        {
            s2ne::fatal("Teleport evidence save write");
        }
    };
    write("teleport-controlled-before.sav", before);
    auto gameStorage = std::make_unique<Prologue>(root, restored.events.difficulty);
    auto& game = *gameStorage;
    game.restore_battle(restored.events, restored.turn);
    game.enable_presentation_timing();
    OriginalMenu font(root);
    bool seen = false;
    unsigned frames = 0;
    auto started = SDL_GetTicks();
    SDL_Event end{ };
    end.type = SDL_EVENT_KEY_DOWN;
    end.key.key = SDLK_F10;
    SDL_PushEvent(&end);
    for (;;)
    {
        if (SDL_GetTicks() - started > 20000 || frames > 5000)
        {
            s2ne::fatal("Teleport SDL timed out");
        }
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_F10)
                {
                    game.end_battle_turn();
                }
                else if (event.key.key == SDLK_ESCAPE)
                {
                    game.cancel_battle_selection();
                }
                else
                {
                    game.advance();
                }
            }
        }
        if (game.teleport_active())
        {
            seen = true;
            if (game.original_save_boundary_ready())
            {
                s2ne::fatal("Teleport save unlocked");
            }
        }
        auto image = game.draw(font);
        if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer))
        {
            s2ne::fatal(SDL_GetError());
        }
        game.frame_presented();
        game.tick(40);
        ++frames;
        if (seen && !game.teleport_active())
        {
            break;
        }
        SDL_Event key{ };
        key.type = SDL_EVENT_KEY_DOWN;
        key.key.key = game.teleport_active() ? SDLK_ESCAPE : SDLK_RETURN;
        SDL_PushEvent(&key);
    }
    auto after = OriginalSaveBridge::capture(game.events, game.battle_turn(), game.camera(), &before);
    write("teleport-controlled-after.sav", after);
    auto loaded = OriginalSave::load(root / "S2NE" / "out" / "teleport-controlled-after.sav", count);
    if (OriginalSave::encode(loaded, count) != OriginalSave::encode(after, count))
    {
        s2ne::fatal("Teleport save roundtrip");
    }
    std::cout << "PASS SDL production side-end45DB10/454ED5 stage60, " << frames << " presented frames, source=" << source.string() << "; controlled pad placement/action exhaustion, not natural navigation E2E; isolated before/after SAVE roundtrip\n";
}
}    //namespace s2ne
