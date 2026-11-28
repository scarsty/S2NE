#include "church.h"

namespace s2ne
{
auto Church::revival_cost(const Save::State& state) const -> unsigned
{
    int value = roles_->combat_value(roles_->units.at(recipients_.at(first_ + row_)));
    unsigned difficulty = state.globals_4f7cd0.at(28);
    int adjustedCost;
    if (difficulty == 1) { adjustedCost = value / 10; }
    else if (difficulty == 2) { adjustedCost = value / 5; }
    else if (difficulty == 4) { adjustedCost = value * 2; }
    else if (difficulty == 5) { adjustedCost = value * 3; }
    else
    {
        adjustedCost = value;
    }

    value = adjustedCost;
    return unsigned(std::max(1, value));
}

auto Church::load_recipients(Save::State& state, bool promoting) -> void
{
    if (!role_panel_)
    {
        role_panel_ = std::make_unique<RolePanel>(root_);
    }
    roles_ = std::make_unique<Events>(root_, state.globals_4f7cd0.at(28));
    roles_->units.clear();
    recipients_.clear();
    std::optional<Promotion> rules;
    if (promoting) { rules.emplace(root_); }
    for (auto& record : state.groups.at(0).units)
    {
        Events::Unit u;
        SaveBridge::decode_unit(record, u);
        roles_->restore_unit_resources(u);
        bool eligible = (u.status_flags & 0x20) && !(u.status_flags & 0x10);
        if (promoting)
        {
            // 441706先4409B7，再排死亡／离队／召唤及WORD8==1；不要求出场80位。
            eligible = rules->eligible(u, u.experience_threshold, int(state.globals_4f7cd0[0]))
                && !(u.status_flags & 0x70) && SaveBridge::get(record, 8) != 1;
            SaveBridge::put(record, 0x113, u.experience);
        }
        roles_->units.push_back(u);
        if (eligible)
        {
            recipients_.push_back(roles_->units.size() - 1);
        }
    }
    first_ = std::min(first_, recipients_.size() > 10 ? recipients_.size() - 10 : 0);
    row_ = recipients_.empty() ? 0 : std::min(row_, recipients_.size() - first_ - 1);
}

Church::Church(const std::filesystem::path& root, Disc disc) : root_(root), font_(root), background_(root, disc, 15), graphics_(root, disc) {}

auto Church::page() const -> Page { return page_; }

auto Church::selected() const -> unsigned { return selected_; }

auto Church::revival_recipients() const -> const std::vector<unsigned long long>& { return recipients_; }

auto Church::revival_selected() const -> size_t { return recipients_.at(first_ + row_); }

auto Church::revival_price(const Save::State& state) const -> unsigned { return revival_cost(state); }

auto Church::blocker() const -> const std::string& { return blocker_; }

auto Church::fee(const Save::State& state) -> unsigned
{
    unsigned cash = ShopMenu::money(state);
    if (cash < 10) { return cash; }
    else if (cash >= 100) { return std::max(100u, cash / 10); }
    else
    {
        return cash / 10;
    }
}

auto Church::supported(const Save::State& state) -> bool { return state.globals_4f7cd0.at(27) != 1 && ShopMenu::money(state) > 0 && fee(state) < 100; }

auto Church::donate(Save::State& state) -> bool
{
    if (!supported(state))
    {
        return false;
    }
    unsigned cost = fee(state);
    auto& metadata = state.groups.at(0).metadata;
    auto get = [&](size_t p)
    {
        uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            v |= uint32_t(metadata.at(p + i)) << (8 * i);
        }
        return v;
    };
    auto put = [&](size_t p, uint32_t value)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            metadata.at(p + i) = uint8_t(value >> (8 * i));
        }
    };
    put(0, get(0) - cost);
    uint32_t total = get(8);
    if (total < 0x1FFFFF)
    {
        put(8, total + cost);
    }
    unsigned times = (unsigned(metadata.at(12)) | (unsigned(metadata.at(13)) << 8)) + 1;
    metadata[12] = uint8_t(times);
    metadata[13] = uint8_t(times >> 8);
    state.globals_4f7cd0[27] = 1;
    return true;
}

auto Church::try_sync_roster(const Save::State& state, Events& events) -> s2ne::Result<bool>
{
    if (state.groups[0].units.size() > events.units.size())
    {
        return s2ne::Result<bool>::failure(make_error("Church persistent roster missing"));
    }
    auto staged = events;
    for (size_t i = 0; i < state.groups[0].units.size(); ++i)
    {
        const auto& record = state.groups[0].units[i];
        if (SaveBridge::get(record, 2) != events.units[i].id)
        {
            return s2ne::Result<bool>::failure(make_error("Church persistent role identity mismatch"));
        }
        SaveBridge::decode_unit(record, staged.units[i]);
    }

    using std::swap;
    swap(events, staged);
    return s2ne::Result<bool>::success(true);
}

auto Church::sync_roster(const Save::State& state, Events& events) -> void
{
    auto result = try_sync_roster(state, events);
    if (!result)
    {
        fatal(result.error());
    }
}

auto Church::set_donation(std::function<bool(Save::State&)> transaction) -> void { donation_ = std::move(transaction); }

auto Church::set_promotion(std::function<bool(Save::State&, size_t, const Promotion::Request&)> transaction) -> void { promotion_commit_ = std::move(transaction); }

auto Church::set_promotion_stop(std::function<void()> stop) -> void { promotion_stop_ = std::move(stop); }

auto Church::promotion_active() const -> bool { return page_ == Page::promotion; }

auto Church::tick(uint64_t milliseconds) -> bool { return promotion_active() && promotion_ && promotion_->tick(milliseconds); }

auto Church::try_promote(const std::filesystem::path& root, const Save::State& before,
    size_t index, const Promotion::Request& request, RandomStream& random, uint32_t seconds) -> s2ne::Result<Save::State>
{
    auto fail = []
    {
        return s2ne::Result<Save::State>::failure(make_error("Original church promotion transaction rejected"));
    };
    if (request.battlefield || index >= before.groups[0].units.size()) { return fail(); }
    auto restored = SaveBridge::try_restore(root, before);
    if (!restored) { return s2ne::Result<Save::State>::failure(restored.error()); }
    auto staged = std::move(restored).value();
    auto& unit = staged.events.units.at(index);
    if ((unit.status_flags & 0x70) || SaveBridge::get(before.groups[0].units[index], 8) == 1) { return fail(); }
    PromotionTransaction transaction(root);
    PromotionTransaction::State candidate{ unit, before.groups[0].inventory, random };
    PromotionTransaction::Context context;
    context.game_mode = int(before.globals_4f7cd0[0]);
    uint8_t metadata = before.groups[0].metadata[4];
    context.rebuild = [&](auto& next)
    {
        auto ready = staged.events.try_restore_unit_resources(next.unit);
        if (!ready)
        {
            report_error(ready.error());
            return false;
        }
        return true;
    };
    context.recalculate_army = [&](auto& next)
    {
        staged.events.units[index] = next.unit;
        auto roles = before.groups[0].role_selectors();
        auto result = transaction.try_aggregate_army_metadata(metadata,
            std::span<const Events::Unit>(staged.events.units).first(roles.size()), roles, 2);
        if (!result)
        {
            report_error(result.error());
            return false;
        }
        metadata = result.value();
        return true;
    };
    auto next = transaction.prepare(candidate, request, context, [seconds]
        {
            return seconds;
        });
    if (!next) { return fail(); }
    auto captured = SaveBridge::try_capture_promotion(staged.events, staged.turn, staged.camera,
        before, index, request.source_kind, next->inventory, metadata);
    if (!captured) { return captured; }
    auto validated = SaveBridge::try_restore(root, captured.value());
    if (!validated) { return s2ne::Result<Save::State>::failure(validated.error()); }
    auto encoded = Save::try_encode(captured.value(), before.groups[0].inventory.size());
    if (!encoded) { return s2ne::Result<Save::State>::failure(encoded.error()); }
    random = next->random;
    return captured;
}

auto Church::promotion_event(const Settings::Event& event, Save::State& state) -> bool
{
    if (!promotion_) { return false; }
    blocker_.clear();
    const auto result = promotion_->event(event);
    if (result.commit_blocked) { blocker_ = "Original church promotion transaction refused"; }
    if (promotion_->page() == Promotion::Page::closed)
    {
        const bool changed = promotion_->return_code() == 2;
        if (promotion_stop_) { promotion_stop_(); }
        promotion_.reset();
        if (changed) { load_recipients(state, true); }
        page_ = recipients_.empty() ? Page::menu : Page::promotion_recipients;
        return changed;
    }
    return false;
}

auto Church::donate_native(const std::filesystem::path& root, Save::State& state, RandomStream& random, uint32_t seconds) -> bool
{
    if (state.globals_4f7cd0.at(27) == 1 || !ShopMenu::money(state))
    {
        return false;
    }
    auto staged = state;
    auto rng = random;
    unsigned cost = fee(staged);
    if (cost >= 100)
    {
        Progression progression(root);
        Promotion promotion(root);
        unsigned difficulty = staged.globals_4f7cd0.at(28);
        int base;
        if (difficulty == 1) { base = 50; }
        else if (difficulty == 2) { base = 40; }
        else if (difficulty == 3) { base = 30; }
        else if (difficulty == 4) { base = 20; }
        else
        {
            base = 10;
        }

        unsigned award = std::min(150u, base + (cost - 100) / 100);
        for (auto& record : staged.groups.at(0).units)
        {
            unsigned flags = record.at(0x33);
            bool excluded = (flags & 0xE0) || SaveBridge::get(record, 8) == 1 || (flags & 0x10);
            unsigned xp = SaveBridge::get(record, 0x113);
            if (!excluded)
            {
                xp = uint16_t(xp + award);
            }
            xp = std::min(999u, xp);
            SaveBridge::put(record, 0x113, xp);
            if (xp >= SaveBridge::get(record, 0x111) && SaveBridge::get(record, 0xB5) < 99)
            {
                Events::Unit unit;
                SaveBridge::decode_unit(record, unit);
                if (!progression.award_native(unit, rng, 0, promotion, [seconds]
                                    {
                                        return seconds;
                                    })
                        .applied)
                {
                    return false;
                }
                SaveBridge::patch_unit(record, unit);
            }
        }
    }
    // Commit the same money/counter branch only after all growth succeeds.
    auto& metadata = staged.groups.at(0).metadata;
    auto get = [&](size_t p)
    {
        uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            v |= uint32_t(metadata.at(p + i)) << (8 * i);
        }
        return v;
    };
    auto put = [&](size_t p, uint32_t v)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            metadata.at(p + i) = uint8_t(v >> (8 * i));
        }
    };
    put(0, get(0) - cost);
    auto total = get(8);
    if (total < 0x1FFFFF)
    {
        put(8, total + cost);
    }
    unsigned n = (unsigned(metadata[12]) | (unsigned(metadata[13]) << 8)) + 1;
    metadata[12] = uint8_t(n);
    metadata[13] = uint8_t(n >> 8);
    staged.globals_4f7cd0[27] = 1;
    state = std::move(staged);
    random = std::move(rng);
    return true;
}

auto Church::scan(unsigned code, Save::State& state) -> bool
{
    if (page_ == Page::promotion)
    {
        Settings::Event event;
        event.type = 1;
        event.scan = code;
        return promotion_event(event, state);
    }
    if (page_ == Page::promotion_recipients)
    {
        if (code == 1)
        {
            page_ = Page::menu;
            return false;
        }
        if (code == 0x48 && first_ + row_)
        {
            if (row_) { --row_; }
            else
            {
                --first_;
            }
        }
        if (code == 0x50 && first_ + row_ + 1 < recipients_.size())
        {
            if (row_ < 9) { ++row_; }
            else
            {
                ++first_;
            }
        }
        if (code == 0x1C || code == 0x39)
        {
            const size_t index = recipients_.at(first_ + row_);
            auto unit = roles_->units.at(index);
            promotion_.emplace(root_);
            if (!promotion_->open(unit, unit.experience_threshold, [&, index](const auto& request)
                    {
                        if (!promotion_commit_ || !promotion_commit_(state, index, request)) { return false; }
                        return true;
                    },
                    false, int(state.globals_4f7cd0[0])))
            {
                promotion_.reset();
                blocker_ = "Original church promotion has no available branch";
                return false;
            }
            page_ = Page::promotion;
        }
        return false;
    }
    if (page_ == Page::closed)
    {
        return false;
    }
    if (page_ == Page::resurrection_confirm)
    {
        if (code == 1)
        {
            confirmation_->cancel();
        }
        else if (code == 0x4B || code == 0x4D || code == 0x0F)
        {
            confirmation_->toggle();
        }
        else if (code == 0x1C || code == 0x39)
        {
            confirmation_->confirm();
        }
        if (auto result = confirmation_->result())
        {
            confirmation_.reset();
            page_ = Page::recipients;
            if (*result)
            {
                unsigned cost = revival_cost(state);
                if (cost > ShopMenu::money(state))
                {
                    return false;
                }
                auto& m = state.groups[0].metadata;
                unsigned cash = ShopMenu::money(state) - cost;
                for (unsigned i = 0; i < 4; ++i)
                {
                    m[i] = uint8_t(cash >> (8 * i));
                }
                state.groups[0].units.at(recipients_.at(first_ + row_)).at(0x33) &= ~0x20u;
                load_recipients(state);
                if (recipients_.empty())
                {
                    page_ = Page::menu;
                }
                return true;
            }
        }
        return false;
    }
    if (page_ == Page::recipients)
    {
        if (message_ == 104)
        {
            if (code)
            {
                message_ = 0;
            }
            return false;
        }
        if ((code == 0x1C || code == 0x39) && revival_cost(state) > ShopMenu::money(state))
        {
            message_ = 104;
            return false;
        }
        if (code == 1)
        {
            page_ = Page::menu;
            return false;
        }
        if (code == 0x48 && first_ + row_)
        {
            if (row_)
            {
                --row_;
            }
            else
            {
                --first_;
            }
        }
        if (code == 0x50 && first_ + row_ + 1 < recipients_.size())
        {
            if (row_ < 9)
            {
                ++row_;
            }
            else
            {
                ++first_;
            }
        }
        if ((code == 0x1C || code == 0x39) && revival_cost(state) <= ShopMenu::money(state))
        {
            confirmation_.emplace(root_, font_, 106, 219, 217 + 20 * int(row_), true);
            page_ = Page::resurrection_confirm;
        }
        return false;
    }
    if (page_ == Page::message)
    {
        if (code)
        {
            page_ = Page::menu;
            message_ = 0;
        }
        return false;
    }
    if (page_ == Page::confirm)
    {
        if (code == 1)
        {
            confirmation_->cancel();
        }
        else if (code == 0x4B || code == 0x4D || code == 0x0F)
        {
            confirmation_->toggle();
        }
        else if (code == 0x1C || code == 0x39)
        {
            confirmation_->confirm();
        }
        if (auto result = confirmation_->result())
        {
            confirmation_.reset();
            page_ = Page::menu;
            if (*result)
            {
                if (donation_ ? donation_(state) : donate(state))
                {
                    message_ = 108;
                    page_ = Page::message;
                    return true;
                }
                blocker_ = "44ACAD transaction refused: shared RNG/growth context unavailable";
            }
        }
        return false;
    }
    if (code == 1)
    {
        page_ = Page::closed;
        return false;
    }
    if (code == 0x4B || code == 0x4D || code == 0x0F)
    {
        selected_ = selected_ % 3 + 1;
    }
    if (code != 0x1C && code != 0x39)
    {
        return false;
    }
    blocker_.clear();
    if (selected_ == 2)
    {
        first_ = row_ = 0;
        load_recipients(state);
        if (recipients_.empty())
        {
            message_ = 102;
            page_ = Page::message;
        }
        else
        {
            page_ = Page::recipients;
        }
        return false;
    }
    if (selected_ != 1)
    {
        first_ = row_ = 0;
        load_recipients(state, true);
        if (recipients_.empty())
        {
            message_ = 210;
            page_ = Page::message;
        }
        else
        {
            page_ = Page::promotion_recipients;
        }
        return true;
    }
    if (state.globals_4f7cd0.at(27) == 1)
    {
        message_ = 101;
        page_ = Page::message;
        return false;
    }
    if (!ShopMenu::money(state))
    {
        message_ = 104;
        page_ = Page::message;
        return false;
    }
    if (!supported(state) && !donation_)
    {
        blocker_ = "44ACAD growth requires shared production RNG transaction";
        return false;
    }
    confirmation_.emplace(root_, font_, 107, 200, 278, true);
    page_ = Page::confirm;
    return false;
}

auto Church::mouse(int x, int y, bool left, bool right, Save::State& state) -> bool
{
    if (page_ == Page::promotion)
    {
        Settings::Event event;
        event.type = left || right ? 0x20 : 8;
        event.x = x;
        event.y = y;
        event.buttons = right ? 2u : unsigned(left);
        return promotion_event(event, state);
    }
    if (right)
    {
        return scan(1, state);
    }
    if (page_ == Page::message)
    {
        return left ? scan(0x1C, state) : false;
    }
    if (page_ == Page::confirm || page_ == Page::resurrection_confirm)
    {
        confirmation_->mouse(float(x), float(y), left);
        return scan(0, state);
    }
    if (page_ == Page::recipients || page_ == Page::promotion_recipients)
    {
        int r = (y - role_panel_->top() - 17) / 20;
        if (x >= 247 && x <= 455 && y >= role_panel_->top() + 17 && r >= 0 && r < 10 && first_ + size_t(r) < recipients_.size())
        {
            if (row_ == size_t(r) && left)
            {
                return scan(0x1C, state);
            }
            row_ = size_t(r);
        }
        return false;
    }
    if (page_ != Page::menu)
    {
        return false;
    }
    for (unsigned i = 0; i < 3; ++i)
    {
        auto r = rectangles[i];
        if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom)
        {
            selected_ = i + 1;
            if (left)
            {
                return scan(0x1C, state);
            }
        }
    }
    return false;
}

auto Church::draw(const Save::State& state) const -> std::vector<uint8_t>
{
    if (page_ == Page::promotion) { return promotion_->draw(background_.background()); }
    if (page_ == Page::promotion_recipients)
    {
        auto image = graphics_.service_panel(153, 209, ShopMenu::money(state), background_, false);
        font_.draw_original_large(image, font_.text_bytes(211), 149, 62, 42);
        role_panel_->draw(image, *roles_, recipients_, first_, row_);
        return image;
    }
    if (page_ == Page::menu || page_ == Page::closed)
    {
        auto image = background_.background();
        auto r = rectangles[selected_ - 1];
        auto label = font_.text_bytes(195 + selected_);
        background_.draw_caption(image, 195 + selected_, (r.left + r.right) / 2 - int(label.size()) * 4, (r.top + r.bottom) / 2 - 30);
        return image;
    }
    if (page_ == Page::recipients || page_ == Page::resurrection_confirm)
    {
        auto image = graphics_.service_panel(153, 209, ShopMenu::money(state), background_);
        role_panel_->draw(image, *roles_, recipients_, first_, row_);
        auto prompt = font_.text_bytes(103);
        {
            auto name = roles_->units.at(recipients_.at(first_ + row_)).name;
            auto n = std::to_string(revival_cost(state));
            for (size_t i = 0; i + 1 < prompt.size(); ++i)
            {
                if (prompt[i] == '%')
                {
                    auto value = prompt[i + 1] == 's' ? name : std::vector<uint8_t>(n.begin(), n.end());
                    prompt.erase(prompt.begin() + i, prompt.begin() + i + 2);
                    prompt.insert(prompt.begin() + i, value.begin(), value.end());
                    i += value.size();
                }
            }
        }
        font_.draw_original_large(image, prompt, 149, 62, 42);
        if (message_ == 104)
        {
            font_.draw_original_large(image, font_.text_bytes(104), 149, 92, 42);
        }
        if (confirmation_)
        {
            confirmation_->draw(image, font_);
        }
        return image;
    }
    auto image = graphics_.service_panel(153, 209, ShopMenu::money(state), background_);
    auto prompt = font_.text_bytes(page_ == Page::confirm ? 105 : message_);
    if (page_ == Page::confirm)
    {
        auto amount = std::to_string(fee(state));
        for (size_t i = 0; i + 1 < prompt.size(); ++i)
        {
            if (prompt[i] == '%' && (prompt[i + 1] == 'd' || prompt[i + 1] == 'u'))
            {
                prompt.erase(prompt.begin() + i, prompt.begin() + i + 2);
                prompt.insert(prompt.begin() + i, amount.begin(), amount.end());
                break;
            }
        }
    }
    font_.draw_original_large(image, prompt, 149, 62, 42);
    if (confirmation_)
    {
        confirmation_->draw(image, font_);
    }
    return image;
}
}    //namespace s2ne
