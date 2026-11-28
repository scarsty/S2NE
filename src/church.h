#pragma once
#include "confirmation.h"
#include "progression.h"
#include "promotion_transaction.h"
#include "role_panel.h"
#include "save_bridge.h"
#include "shop_menu.h"
namespace s2ne
{
class Church
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
    static constexpr std::array<Interbattle::Rect, 3> rectangles{ { { 75, 314, 163, 383 }, { 268, 305, 382, 400 }, { 495, 204, 629, 317 } } };

private:
    std::filesystem::path root_;
    Menu font_;
    Interbattle background_;
    ShopMenu graphics_;
    std::optional<Confirmation> confirmation_;
    Page page_ = Page::menu;
    unsigned selected_ = 1, message_ = 0;
    std::string blocker_;
    std::function<bool(Save::State&)> donation_;
    std::function<bool(Save::State&, size_t, const Promotion::Request&)> promotion_commit_;
    std::function<void()> promotion_stop_;
    std::optional<Promotion> promotion_;
    std::unique_ptr<RolePanel> role_panel_;
    std::unique_ptr<Events> roles_;
    std::vector<size_t> recipients_;
    size_t first_ = 0, row_ = 0;
    unsigned revival_cost(const Save::State& state) const;
    void load_recipients(Save::State& state, bool promoting = false);

public:
    explicit Church(const std::filesystem::path& root, Disc disc = Disc::one);
    Page page() const;
    unsigned selected() const;
    const std::vector<unsigned long long>& revival_recipients() const;
    size_t revival_selected() const;
    unsigned revival_price(const Save::State& state) const;
    const std::string& blocker() const;
    static unsigned fee(const Save::State& state);
    static bool supported(const Save::State& state);
    static bool donate(Save::State& state);
    static s2ne::Result<bool> try_sync_roster(const Save::State& state, Events& events);
    static void sync_roster(const Save::State& state, Events& events);
    void set_donation(std::function<bool(Save::State&)> transaction);
    void set_promotion(std::function<bool(Save::State&, size_t, const Promotion::Request&)> transaction);
    void set_promotion_stop(std::function<void()> stop);
    bool promotion_active() const;
    bool tick(uint64_t milliseconds);
    static s2ne::Result<Save::State> try_promote(const std::filesystem::path& root, const Save::State& before,
        size_t index, const Promotion::Request& request, RandomStream& random, uint32_t seconds);
    bool promotion_event(const Settings::Event& event, Save::State& state);
    static bool donate_native(const std::filesystem::path& root, Save::State& state, RandomStream& random, uint32_t seconds);
    bool scan(unsigned code, Save::State& state);
    bool mouse(int x, int y, bool left, bool right, Save::State& state);
    std::vector<uint8_t> draw(const Save::State& state) const;
};
}    //namespace s2ne
