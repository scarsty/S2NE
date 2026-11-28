#pragma once
#include "original_confirmation.h"
#include "original_progression.h"
#include "original_promotion_transaction.h"
#include "original_role_panel.h"
#include "original_save_bridge.h"
#include "original_shop_menu.h"
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
        resurrection_confirm,
        promotion_recipients,
        promotion
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
    std::function<bool(OriginalSave::State&, size_t, const OriginalPromotion::Request&)> promotion_commit_;
    std::function<void()> promotion_stop_;
    std::optional<OriginalPromotion> promotion_;
    std::unique_ptr<OriginalRolePanel> role_panel_;
    std::unique_ptr<OriginalEvents> roles_;
    std::vector<size_t> recipients_;
    size_t first_ = 0, row_ = 0;
    unsigned revival_cost(const OriginalSave::State& state) const;
    void load_recipients(OriginalSave::State& state, bool promoting = false);

public:
    explicit OriginalChurch(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one);
    Page page() const;
    unsigned selected() const;
    const std::vector<unsigned long long>& revival_recipients() const;
    size_t revival_selected() const;
    unsigned revival_price(const OriginalSave::State& state) const;
    const std::string& blocker() const;
    static unsigned fee(const OriginalSave::State& state);
    static bool supported(const OriginalSave::State& state);
    static bool donate(OriginalSave::State& state);
    static s2ne::Result<bool> try_sync_roster(const OriginalSave::State& state, OriginalEvents& events);
    static void sync_roster(const OriginalSave::State& state, OriginalEvents& events);
    void set_donation(std::function<bool(OriginalSave::State&)> transaction);
    void set_promotion(std::function<bool(OriginalSave::State&, size_t, const OriginalPromotion::Request&)> transaction);
    void set_promotion_stop(std::function<void()> stop);
    bool promotion_active() const;
    bool tick(uint64_t milliseconds);
    static s2ne::Result<OriginalSave::State> try_promote(const std::filesystem::path& root, const OriginalSave::State& before,
        size_t index, const OriginalPromotion::Request& request, OriginalRandom& random, uint32_t seconds);
    bool promotion_event(const OriginalSettings::Event& event, OriginalSave::State& state);
    static bool donate_native(const std::filesystem::path& root, OriginalSave::State& state, OriginalRandom& random, uint32_t seconds);
    bool scan(unsigned code, OriginalSave::State& state);
    bool mouse(int x, int y, bool left, bool right, OriginalSave::State& state);
    std::vector<uint8_t> draw(const OriginalSave::State& state) const;
};
}    //namespace s2ne
