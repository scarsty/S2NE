#pragma once
#include "original_confirmation.hpp"
#include "original_progression.hpp"
#include "original_role_panel.hpp"
#include "original_save_bridge.hpp"
#include "original_shop_menu.hpp"
namespace s2ne
{
class OriginalChurch
{
public:
    enum class Page
    {
        menu,
        confirm,
        message,
        closed,
        recipients,
        resurrection_confirm
    };
    static constexpr std::array<OriginalInterbattle::Rect, 3> rectangles{ { { 75, 314, 163, 383 }, { 268, 305, 382, 400 }, { 495, 204, 629, 317 } } };

private:
    std::filesystem::path root_;
    OriginalMenu font_;
    OriginalInterbattle background_;
    OriginalShopMenu graphics_;
    std::optional<OriginalConfirmation> confirmation_;
    Page page_ = Page::menu;
    unsigned selected_ = 1, message_ = 0;
    std::string blocker_;
    std::function<bool(OriginalSave::State&)> donation_;
    std::unique_ptr<OriginalRolePanel> role_panel_;
    std::unique_ptr<OriginalEvents> roles_;
    std::vector<size_t> recipients_;
    size_t first_ = 0, row_ = 0;
    unsigned revival_cost(const OriginalSave::State& state) const
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
    void load_recipients(const OriginalSave::State& state)
    {
        if (!role_panel_)
        {
            role_panel_ = std::make_unique<OriginalRolePanel>(root_);
        }
        roles_ = std::make_unique<OriginalEvents>(root_, state.globals_4f7cd0.at(28));
        roles_->units.clear();
        recipients_.clear();
        for (const auto& record : state.groups.at(0).units)
        {
            OriginalEvents::Unit u;
            OriginalSaveBridge::decode_unit(record, u);
            roles_->restore_unit_resources(u);
            roles_->units.push_back(u);
            if ((u.status_flags & 0x20) && !(u.status_flags & 0x10))
            {
                recipients_.push_back(roles_->units.size() - 1);
            }
        }
        first_ = std::min(first_, recipients_.size() > 10 ? recipients_.size() - 10 : 0);
        row_ = recipients_.empty() ? 0 : std::min(row_, recipients_.size() - first_ - 1);
    }

public:
    explicit OriginalChurch(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one) : root_(root), font_(root), background_(root, disc, 15), graphics_(root, disc) { }
    Page page() const { return page_; }
    const std::string& blocker() const { return blocker_; }
    static unsigned fee(const OriginalSave::State& state)
    {
        unsigned cash = OriginalShopMenu::money(state);
        if (cash < 10) { return cash; }
        else if (cash >= 100) { return std::max(100u, cash / 10); }
        else
        {
            return cash / 10;
        }
    }
    static bool supported(const OriginalSave::State& state) { return state.globals_4f7cd0.at(27) != 1 && OriginalShopMenu::money(state) > 0 && fee(state) < 100; }
    static bool donate(OriginalSave::State& state)
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
    static s2ne::Result<bool> try_sync_roster(const OriginalSave::State& state, OriginalEvents& events)
    {
        if (state.groups[0].units.size() > events.units.size())
        {
            return s2ne::Result<bool>::failure(make_error("Church persistent roster missing"));
        }
        auto staged = events;
        for (size_t i = 0; i < state.groups[0].units.size(); ++i)
        {
            const auto& record = state.groups[0].units[i];
            if (OriginalSaveBridge::get(record, 2) != events.units[i].id)
            {
                return s2ne::Result<bool>::failure(make_error("Church persistent role identity mismatch"));
            }
            OriginalSaveBridge::decode_unit(record, staged.units[i]);
        }

        using std::swap;
        swap(events, staged);
        return s2ne::Result<bool>::success(true);
    }
    static void sync_roster(const OriginalSave::State& state, OriginalEvents& events)
    {
        auto result = try_sync_roster(state, events);
        if (!result)
        {
            report_error(result.error());
        }
    }
    void set_donation(std::function<bool(OriginalSave::State&)> transaction) { donation_ = std::move(transaction); }
    static bool donate_native(const std::filesystem::path& root, OriginalSave::State& state, OriginalRandom& random, uint32_t seconds)
    {
        if (state.globals_4f7cd0.at(27) == 1 || !OriginalShopMenu::money(state))
        {
            return false;
        }
        auto staged = state;
        auto rng = random;
        unsigned cost = fee(staged);
        if (cost >= 100)
        {
            OriginalProgression progression(root);
            OriginalPromotion promotion(root);
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
                bool excluded = (flags & 0xE0) || OriginalSaveBridge::get(record, 8) == 1 || (flags & 0x10);
                unsigned xp = OriginalSaveBridge::get(record, 0x113);
                if (!excluded)
                {
                    xp = uint16_t(xp + award);
                }
                xp = std::min(999u, xp);
                OriginalSaveBridge::put(record, 0x113, xp);
                if (xp >= OriginalSaveBridge::get(record, 0x111) && OriginalSaveBridge::get(record, 0xB5) < 99)
                {
                    OriginalEvents::Unit unit;
                    OriginalSaveBridge::decode_unit(record, unit);
                    if (!progression.award_native(unit, rng, 0, promotion, [seconds]
                                        {
                                            return seconds;
                                        })
                            .applied)
                    {
                        return false;
                    }
                    OriginalSaveBridge::patch_unit(record, unit);
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
    bool scan(unsigned code, OriginalSave::State& state)
    {
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
                    if (cost > OriginalShopMenu::money(state))
                    {
                        return false;
                    }
                    auto& m = state.groups[0].metadata;
                    unsigned cash = OriginalShopMenu::money(state) - cost;
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
            if ((code == 0x1C || code == 0x39) && revival_cost(state) > OriginalShopMenu::money(state))
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
            if ((code == 0x1C || code == 0x39) && revival_cost(state) <= OriginalShopMenu::money(state))
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
            blocker_ = "41430B(1)/45954D object transaction not restored";
            return false;
        }
        if (state.globals_4f7cd0.at(27) == 1)
        {
            message_ = 101;
            page_ = Page::message;
            return false;
        }
        if (!OriginalShopMenu::money(state))
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
    bool mouse(int x, int y, bool left, bool right, OriginalSave::State& state)
    {
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
        if (page_ == Page::recipients)
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
    std::vector<uint8_t> draw(const OriginalSave::State& state) const
    {
        if (page_ == Page::menu || page_ == Page::closed)
        {
            auto image = background_.background();
            auto r = rectangles[selected_ - 1];
            auto label = font_.text_bytes(195 + selected_);
            font_.draw_small(image, label, (r.left + r.right) / 2 - int(label.size()) * 4, (r.top + r.bottom) / 2 - 30, 3);
            return image;
        }
        if (page_ == Page::recipients || page_ == Page::resurrection_confirm)
        {
            auto image = graphics_.service_panel(153, 209, OriginalShopMenu::money(state), background_);
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
        auto image = graphics_.service_panel(153, 209, OriginalShopMenu::money(state), background_);
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
};
}    //namespace s2ne
