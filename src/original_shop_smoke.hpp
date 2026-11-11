#pragma once
#include "original_church.hpp"
#include "original_inn.hpp"
#include "original_shop_menu.hpp"
#include "runtime_error.hpp"
namespace s2ne
{
inline void original_shop_smoke(const std::filesystem::path& root, SDL_Renderer* renderer, SDL_Texture* texture)
{
    std::ifstream f(root / "ITEMSP.BIN", std::ios::binary);
    std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(f), { });
    auto count = OriginalSave::item_count(bytes);
    OriginalShop shop(root);
    OriginalSave::State state;
    bool found = false;
    for (const auto& file : std::filesystem::directory_iterator(root / "SAVE"))
    {
        if (file.path().extension() != L".SAV")
        {
            continue;
        }
        auto raw = OriginalSave::load(file.path(), count);
        if (!shop.sale_items(raw).empty())
        {
            state = std::move(raw);
            found = true;
            break;
        }
    }
    if (!found)
    {
        s2ne::fatal("Shop smoke requires genuine SAVE inventory");
    }
    auto before = state;
    OriginalInterbattle hub(root);
    std::optional<OriginalShopMenu> page;
    unsigned phase = 0, frames = 0, id = 0, price = 0;
    uint64_t started = SDL_GetTicks();
    while (phase < 11)
    {
        if (SDL_GetTicks() - started > 10000)
        {
            s2ne::fatal("Normal SDL shop input timed out");
        }
        SDL_Event injected{ };
        injected.type = SDL_EVENT_KEY_DOWN;
        if (phase == 0 || phase == 2 || phase == 3 || phase == 4) { injected.key.key = SDLK_TAB; }
        else
        {
            if (phase >= 8) { injected.key.key = SDLK_ESCAPE; }
            else
            {
                injected.key.key = SDLK_RETURN;
            }
        }
        if (!SDL_PushEvent(&injected))
        {
            s2ne::fatal(SDL_GetError());
        }
        SDL_Event event;
        bool consumed = false;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                int code;
                if (event.key.key == SDLK_TAB) { code = 0x0F; }
                else if (event.key.key == SDLK_ESCAPE) { code = 1; }
                else
                {
                    code = 0x1C;
                }

                if (page)
                {
                    if (page->page() == OriginalShopMenu::Page::quantity && code == 0x1C)
                    {
                        id = page->item();
                        price = shop.sale_price(state, id);
                    }
                    page->scan(code, state);
                    if (page->page() == OriginalShopMenu::Page::closed)
                    {
                        page.reset();
                        hub.dispatch([](auto)
                            {
                                return OriginalInterbattle::Result::returned;
                            });
                    }
                }
                else
                {
                    hub.scan(code);
                }
                consumed = true;
            }
        }
        if (!page)
        {
            if (auto request = hub.request(); request && request->option == 2)
            {
                page.emplace(root);
            }
        }
        auto image = page ? page->draw(state) : hub.draw();
        if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer))
        {
            s2ne::fatal(SDL_GetError());
        }
        ++frames;
        if (consumed)
        {
            ++phase;
        }
    }
    if (!id || page || hub.pending() || hub.closed())
    {
        s2ne::fatal("Shop smoke did not return to hub");
    }
    auto expected = before;
    if (!shop.sell(expected, id, 1) || OriginalSave::encode(expected, count) != OriginalSave::encode(state, count))
    {
        s2ne::fatal("SDL sale exact native delta mismatch");
    }
    auto path = root / "S2NE" / "out" / "shop-sdl-roundtrip.sav";
    auto data = OriginalSave::encode(state, count);
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        if (!out)
        {
            s2ne::fatal("Shop smoke save write");
        }
    }
    if (OriginalSave::encode(OriginalSave::load(path, count), count) != data)
    {
        s2ne::fatal("SDL sale saved file roundtrip mismatch");
    }
    {
        auto raw = before;
        OriginalShopMenu fade(root);
        fade.enable_presentation_timing();
        for (unsigned i = 0; i < 3; ++i)
        {
            fade.scan(0x0F, raw);
        }
        fade.scan(0x1C, raw);
        unsigned frames = 0;
        auto baseline = OriginalSave::encode(raw, count);
        while (fade.transitioning())
        {
            SDL_Event key{ };
            key.type = SDL_EVENT_KEY_DOWN;
            key.key.key = SDLK_ESCAPE;
            SDL_PushEvent(&key);
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    fade.scan(1, raw);
                }
            }
            auto image = fade.draw(raw);
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer))
            {
                s2ne::fatal(SDL_GetError());
            }
            fade.frame_presented();
            ++frames;
        }
        if (frames != 16 || fade.page() != OriginalShopMenu::Page::sale || OriginalSave::encode(raw, count) != baseline)
        {
            s2ne::fatal("411587 SDL fade/input lock");
        }
        std::cout << "SDL411587 original16 palette steps/input lock PASS\n";
    }
    bool bought = false;
    for (const auto& file : std::filesystem::directory_iterator(root / "SAVE"))
    {
        if (file.path().extension() != L".SAV")
        {
            continue;
        }
        auto candidate = OriginalSave::load(file.path(), count);
        OriginalShopMenu buy(root);
        buy.scan(0x0F, candidate);
        buy.scan(0x0F, candidate);
        buy.scan(0x1C, candidate);
        if (buy.page() != OriginalShopMenu::Page::sale || !buy.item())
        {
            continue;
        }
        buy.scan(0x1C, candidate);
        if (buy.page() != OriginalShopMenu::Page::quantity)
        {
            continue;
        }
        auto baseline = candidate;
        unsigned item = buy.item();
        OriginalShopMenu live(root);
        for (auto key : { SDLK_TAB, SDLK_TAB, SDLK_RETURN, SDLK_RETURN, SDLK_ESCAPE, SDLK_RETURN, SDLK_RETURN, SDLK_ESCAPE, SDLK_ESCAPE })
        {
            SDL_Event input{ };
            input.type = SDL_EVENT_KEY_DOWN;
            input.key.key = key;
            if (!SDL_PushEvent(&input))
            {
                s2ne::fatal(SDL_GetError());
            }
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    int code;
                    if (event.key.key == SDLK_TAB) { code = 0x0F; }
                    else if (event.key.key == SDLK_ESCAPE) { code = 1; }
                    else
                    {
                        code = 0x1C;
                    }

                    live.scan(code, candidate);
                }
            }
            auto image = live.draw(candidate);
            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer))
            {
                s2ne::fatal(SDL_GetError());
            }
        }
        auto expected = baseline;
        if (live.page() != OriginalShopMenu::Page::closed || !shop.buy(expected, item, 1) || OriginalSave::encode(candidate, count) != OriginalSave::encode(expected, count))
        {
            s2ne::fatal("SDL original buy delta/return");
        }
        auto encoded = OriginalSave::encode(candidate, count);
        auto buyPath = root / "S2NE" / "out" / "shop-buy-sdl-roundtrip.sav";
        {
            std::ofstream output(buyPath, std::ios::binary);
            output.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
            if (!output)
            {
                s2ne::fatal("SDL buy save write");
            }
        }
        if (OriginalSave::encode(OriginalSave::load(buyPath, count), count) != encoded)
        {
            s2ne::fatal("SDL original buy file roundtrip");
        }
        bought = true;
        break;
    }
    if (!bought)
    {
        s2ne::fatal("No genuine town consumable offer affordable for SDL buying");
    }
    auto serviceInput = [&](auto& service, auto& save, SDL_Keycode key)
    {
        SDL_Event input{ };
        input.type = SDL_EVENT_KEY_DOWN;
        input.key.key = key;
        if (!SDL_PushEvent(&input))
        {
            s2ne::fatal(SDL_GetError());
        }
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                int scanCode;
                if (event.key.key == SDLK_ESCAPE) { scanCode = 1; }
                else if (event.key.key == SDLK_TAB) { scanCode = 0x0F; }
                else if (event.key.key == SDLK_SPACE) { scanCode = 0x39; }
                else
                {
                    scanCode = 0x1C;
                }
                service.scan(scanCode,
                    save);
            }
        }
        auto image = service.draw(save);
        if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer))
        {
            s2ne::fatal(SDL_GetError());
        }
    };
    {
        auto save = before;
        for (unsigned option : { 5u, 7u, 8u })
        {
            OriginalInn inn(root);
            inn.enable_presentation_timing();
            auto area = OriginalInn::rectangles[option - 1];
            SDL_Event click{ };
            click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            click.button.button = SDL_BUTTON_LEFT;
            click.button.x = float((area.left + area.right) / 2);
            click.button.y = float((area.top + area.bottom) / 2);
            if (!SDL_PushEvent(&click)) { s2ne::fatal(SDL_GetError()); }
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    inn.mouse(int(event.button.x), int(event.button.y), true, false, save);
                }
            }
            if (!inn.blocker().empty()) { s2ne::fatal(inn.blocker()); }
            if (option != 8 && inn.equipment_active())
            {
                for (unsigned step = 0; step < 16; ++step)
                {
                    serviceInput(inn, save, SDLK_ESCAPE);
                    if (!inn.equipment_active()) { s2ne::fatal("SDL inn palette input lock"); }
                    inn.frame_presented();
                }
            }
            serviceInput(inn, save, SDLK_ESCAPE);
            if (inn.closed() || inn.equipment_active() || inn.title_confirmation_active() || OriginalSave::encode(save, count) != OriginalSave::encode(before, count))
            {
                s2ne::fatal("SDL inn equipment/item/title cancel return");
            }
            if (option == 8)
            {
                inn.mouse(int(click.button.x), int(click.button.y), true, false, save);
                serviceInput(inn, save, SDLK_RETURN);
                if (!inn.title_requested() || !inn.closed()) { s2ne::fatal("SDL inn title confirm"); }
            }
        }
        std::cout << "SDL inn410FFE/411034 equipment/item mouse entry,16 presents/input lock/cancel and41104F title cancel/confirm PASS (genuine SAVE)\n";
    }
    {
        auto save = before;
        OriginalInn automatic(root);
        for (unsigned i = 0; i < 9; ++i)
        {
            serviceInput(automatic, save, SDLK_TAB);
        }
        serviceInput(automatic, save, SDLK_RETURN);
        serviceInput(automatic, save, SDLK_ESCAPE);
        if (OriginalSave::encode(save, count) != OriginalSave::encode(before, count))
        {
            s2ne::fatal("SDL auto equipment cancel");
        }
        serviceInput(automatic, save, SDLK_RETURN);
        serviceInput(automatic, save, SDLK_RETURN);
        auto expected = before;
        OriginalEquipment(root).auto_equip(expected);
        if (!automatic.take_equipment_changed() || automatic.closed() || OriginalSave::encode(save, count) != OriginalSave::encode(expected, count))
        {
            s2ne::fatal("SDL auto equipment original transaction/return");
        }
        auto file = root / "S2NE" / "out" / "auto-equipment-sdl.sav";
        auto encoded = OriginalSave::encode(save, count);
        {
            std::ofstream out(file, std::ios::binary);
            out.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
        }
        if (OriginalSave::encode(OriginalSave::load(file, count), count) != encoded)
        {
            s2ne::fatal("SDL auto equipment save roundtrip");
        }
        std::filesystem::remove(file);
        std::cout << "SDL451ABC genuine SAVE original confirmation/cancel/submit/return and file roundtrip PASS\n";
    }
    {
        auto save = before;
        OriginalInn inn(root);
        auto settings = OriginalSettings::load_state(root);
        settings.difficulty = 3;
        inn.prepare_settings(settings);
        for (unsigned i = 0; i < 8; ++i)
        {
            serviceInput(inn, save, SDLK_TAB);
        }
        serviceInput(inn, save, SDLK_RETURN);
        if (!inn.settings_active())
        {
            s2ne::fatal("SDL inn settings entry");
        }
        serviceInput(inn, save, SDLK_ESCAPE);
        auto returned = inn.take_settings();
        if (!returned || returned->difficulty != 3 || inn.closed() || OriginalSave::encode(save, count) != OriginalSave::encode(before, count))
        {
            s2ne::fatal("SDL inn mode2 settings return");
        }
        std::cout << "SDL inn434103(2) native settings entry/return and unchanged SAVE PASS\n";
    }
    {
        auto save = before;
        for (auto& role : save.groups[0].units)
        {
            role[0x33] &= ~0x20u;
        }
        if (save.groups[0].units.empty())
        {
            s2ne::fatal("SDL revival roster missing");
        }
        save.groups[0].units.front()[0x33] = (save.groups[0].units.front()[0x33] | 0x20) & ~0x10u;
        for (unsigned i = 0; i < 4; ++i)
        {
            save.groups[0].metadata[i] = uint8_t(1000000u >> (8 * i));
        }
        auto expected = save;
        OriginalChurch revival(root);
        for (auto key : { SDLK_TAB, SDLK_RETURN, SDLK_RETURN, SDLK_ESCAPE, SDLK_RETURN, SDLK_RETURN, SDLK_ESCAPE })
        {
            serviceInput(revival, save, key);
        }
        expected.groups[0].units.front()[0x33] &= ~0x20u;
        std::copy_n(save.groups[0].metadata.begin(), 4, expected.groups[0].metadata.begin());
        if (OriginalSave::encode(expected, count) != OriginalSave::encode(save, count) || revival.page() != OriginalChurch::Page::closed || OriginalShopMenu::money(save) >= 1000000)
        {
            s2ne::fatal("SDL revival exact original mutation");
        }
        std::cout << "SDL41430B resurrection recipient/native217 page, cancel/confirm/return and exact HP/inventory preservation PASS (controlled dead role/money)\n";
    }
    {
        bool done = false;
        OriginalEquipment equipment(root);
        for (const auto& file : std::filesystem::directory_iterator(root / "SAVE"))
        {
            if (file.path().extension() != L".SAV")
            {
                continue;
            }
            auto save = OriginalSave::load(file.path(), count);
            OriginalShopMenu probe(root);
            probe.scan(0x1C, save);
            if (probe.page() != OriginalShopMenu::Page::sale || !probe.item())
            {
                continue;
            }
            for (unsigned i = 0; i < count; ++i)
            {
                unsigned id = probe.item();
                auto recipients = equipment.recipients(save, id);
                probe.scan(0x1C, save);
                if (probe.page() == OriginalShopMenu::Page::equipment && !recipients.empty())
                {
                    probe.scan(1, save);
                    {
                        auto scrolling = save;
                        scrolling.groups[0].units.assign(12, save.groups[0].units[recipients.front()]);
                        auto encoded = OriginalSave::encode(scrolling, count);
                        OriginalShopMenu cross(root);
                        serviceInput(cross, scrolling, SDLK_RETURN);
                        for (unsigned j = 0; j < i; ++j)
                        {
                            cross.scan(0x4D, scrolling);
                        }
                        serviceInput(cross, scrolling, SDLK_RETURN);
                        int top = 480 - cross.panel_height(218);
                        for (int y : { top + 190, top + 70, top + 76 })
                        {
                            SDL_Event event{ };
                            event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                            event.button.button = SDL_BUTTON_LEFT;
                            event.button.x = 248;
                            event.button.y = float(y);
                            if (!SDL_PushEvent(&event))
                            {
                                s2ne::fatal(SDL_GetError());
                            }
                            while (SDL_PollEvent(&event))
                            {
                                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                                {
                                    cross.mouse(int(event.button.x), int(event.button.y), true, false, scrolling);
                                }
                            }
                            auto image = cross.draw(scrolling);
                            if (!SDL_UpdateTexture(texture, nullptr, image.data(), 640 * 4) || !SDL_RenderTexture(renderer, texture, nullptr, nullptr) || !SDL_RenderPresent(renderer))
                            {
                                s2ne::fatal(SDL_GetError());
                            }
                            if (y == top + 190 && (cross.recipient_first() != 7 || cross.recipient_index() != 11))
                            {
                                s2ne::fatal("SDL equipment track cross page");
                            }
                        }
                        serviceInput(cross, scrolling, SDLK_ESCAPE);
                        if (OriginalSave::encode(scrolling, count) != encoded)
                        {
                            s2ne::fatal("SDL scroll cancellation mutated SAVE");
                        }
                        std::cout << "SDL4125FC original track bottom/up/top and immutable cancel PASS (controlled 12 identical original records)\n";
                    }
                    auto baseline = save;
                    OriginalShopMenu live(root);
                    serviceInput(live, save, SDLK_RETURN);
                    for (unsigned j = 0; j < i; ++j)
                    {
                        SDL_Event e{ };
                        e.type = SDL_EVENT_KEY_DOWN;
                        e.key.key = SDLK_RIGHT;
                        SDL_PushEvent(&e);
                        while (SDL_PollEvent(&e))
                        {
                            if (e.type == SDL_EVENT_KEY_DOWN)
                            {
                                live.scan(0x4D, save);
                            }
                        }
                    }
                    for (auto key : { SDLK_RETURN, SDLK_TAB, SDLK_RETURN, SDLK_ESCAPE, SDLK_RETURN, SDLK_RETURN, SDLK_ESCAPE, SDLK_ESCAPE })
                    {
                        serviceInput(live, save, key);
                    }
                    auto expected = baseline;
                    if (!shop.buy(expected, id, 1) || !equipment.equip(expected, recipients.front(), id) || OriginalSave::encode(save, count) != OriginalSave::encode(expected, count) || live.page() != OriginalShopMenu::Page::closed)
                    {
                        s2ne::fatal("SDL equipment replacement delta/return");
                    }
                    auto encoded = OriginalSave::encode(save, count);
                    auto file = root / "S2NE" / "out" / "equipment-sdl-roundtrip.sav";
                    {
                        std::ofstream out(file, std::ios::binary);
                        out.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
                    }
                    if (OriginalSave::encode(OriginalSave::load(file, count), count) != encoded)
                    {
                        s2ne::fatal("SDL equipment file roundtrip");
                    }
                    std::cout << "SDL equipment genuine SAVE offer=" << id << " recipient=" << recipients.front() << " original comparison/cancel/replace/return and save roundtrip PASS\n";
                    done = true;
                    break;
                }
                if (probe.page() == OriginalShopMenu::Page::equipment)
                {
                    probe.scan(1, save);
                }
                unsigned old = probe.item();
                probe.scan(0x4D, save);
                if (probe.item() == old)
                {
                    break;
                }
            }
            if (done)
            {
                break;
            }
        }
        if (!done)
        {
            s2ne::fatal("SDL genuine equipment recipient missing");
        }
    }
    bool heard = false;
    for (const auto& file : std::filesystem::directory_iterator(root / "SAVE"))
    {
        if (file.path().extension() != L".SAV")
        {
            continue;
        }
        auto save = OriginalSave::load(file.path(), count);
        OriginalInn inn(root);
        if (inn.town_rumor(save, 0).empty() || inn.town_rumor(save, 1).empty())
        {
            continue;
        }
        auto prior = OriginalSave::encode(save, count);
        for (auto key : { SDLK_TAB, SDLK_RETURN })
        {
            serviceInput(inn, save, key);
        }
        if (!inn.reading())
        {
            s2ne::fatal("SDL inn rumor entry");
        }
        serviceInput(inn, save, SDLK_RETURN);
        for (unsigned i = 0; i < 4; ++i)
        {
            serviceInput(inn, save, SDLK_TAB);
        }
        serviceInput(inn, save, SDLK_RETURN);
        if (!inn.roster_active())
        {
            s2ne::fatal("SDL inn roster entry");
        }
        serviceInput(inn, save, SDLK_SPACE);
        for (unsigned i = 0; i < 3; ++i)
        {
            serviceInput(inn, save, SDLK_ESCAPE);
        }
        if (!inn.closed() || OriginalSave::encode(save, count) != prior)
        {
            s2ne::fatal("SDL inn return/delta");
        }
        heard = true;
        break;
    }
    if (!heard)
    {
        s2ne::fatal("SDL inn genuine town not found");
    }
    auto churchSave = before;
    std::fill_n(churchSave.groups[0].metadata.begin(), 4, uint8_t(0));
    churchSave.groups[0].metadata[0] = 99;
    churchSave.globals_4f7cd0[27] = 0;
    auto churchExpected = churchSave;
    OriginalChurch church(root);
    for (auto key : { SDLK_RETURN, SDLK_ESCAPE, SDLK_RETURN, SDLK_RETURN, SDLK_RETURN, SDLK_ESCAPE })
    {
        serviceInput(church, churchSave, key);
    }
    if (!OriginalChurch::donate(churchExpected) || OriginalSave::encode(churchSave, count) != OriginalSave::encode(churchExpected, count) || church.page() != OriginalChurch::Page::closed)
    {
        s2ne::fatal("SDL church cancel/commit/return");
    }
    auto largeSave = before;
    for (unsigned i = 0; i < 4; ++i)
    {
        largeSave.groups[0].metadata[i] = uint8_t(1000u >> (8 * i));
    }
    largeSave.globals_4f7cd0[27] = 0;
    auto largeExpected = largeSave;
    OriginalRandom largeRandom({ 42, 0 }), expectedRandom({ 42, 0 });
    OriginalChurch largeChurch(root);
    largeChurch.set_donation([&](auto& save)
        {
            return OriginalChurch::donate_native(root, save, largeRandom, 1234);
        });
    for (auto key : { SDLK_RETURN, SDLK_ESCAPE, SDLK_RETURN, SDLK_RETURN, SDLK_RETURN, SDLK_ESCAPE })
    {
        serviceInput(largeChurch, largeSave, key);
    }
    if (!OriginalChurch::donate_native(root, largeExpected, expectedRandom, 1234) || OriginalSave::encode(largeSave, count) != OriginalSave::encode(largeExpected, count) || largeRandom.state() != expectedRandom.state() || largeChurch.page() != OriginalChurch::Page::closed)
    {
        s2ne::fatal("SDL large church native transaction");
    }
    std::cout << "SDL large donation normal confirm/cancel/result/return + native XP/growth RNG exact delta PASS (controlled1000-money fixture)\n";
    std::cout << "SDL inn original town rumor wait/return and church controlled99-money fee9 confirm/cancel/result/return PASS; church is controlled fixture, not natural qualifying SAVE\n";
    std::cout << "SDL original town-gated consumable buy confirmation + exact original save delta/roundtrip PASS\n";
    std::cout << "SDL normal hub/shop/sale/quantity/confirm/return input PASS; genuine inventory item=" << id << " price=" << price << " frames=" << frames << " exact native delta + saved file roundtrip; isolated hub entry, not campaign clear\n";
}
}    //namespace s2ne
