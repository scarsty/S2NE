#pragma once
#include "original_confirmation.h"
#include "original_equipment.h"
#include "original_interbattle.h"
#include "original_shop.h"
#include "original_sprite.h"
#include "original_town_data.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
namespace s2ne
{
class OriginalShopMenu
{
public:
    enum class Page
    {
        shop,
        sale,
        quantity,
        closed,
        equipment,
        equipment_confirm
    };
    static constexpr std::array<OriginalInterbattle::Rect, 4> rectangles{ { { 124, 265, 391, 476 }, { 496, 92, 609, 200 }, { 227, 105, 393, 245 }, { 404, 137, 494, 316 } } };

private:
    std::filesystem::path root_;
    OriginalShop shop_;
    OriginalMenu font_;
    OriginalInterbattle background_;
    SharedBytes cel_, pal_;
    std::vector<unsigned> items_;
    std::vector<std::vector<uint8_t>> help_;
    Page page_ = Page::shop;
    int option_ = 1, first_ = 0, row_ = 0;
    unsigned quantity_ = 1;
    bool insufficient_ = false;
    unsigned held_scroll_ = 0, held_stamp_ = 0;
    unsigned clock_cs_ = 0;
    std::optional<std::pair<int, int>> cursor_request_;
    mutable std::vector<uint8_t> cached_panel_;
    mutable std::vector<uint8_t> cached_key_;
    mutable int cached_first_ = -1;
    bool buying_ = false;
    bool presentation_driven_ = false;
    unsigned fade_step_ = 16;
    void begin_entry();
    std::vector<uint8_t> fade(std::vector<uint8_t> image, FullWidthCanvas* canvas) const;
    std::vector<uint8_t> towns_;
    std::unique_ptr<OriginalEquipment> equipment_;
    std::optional<OriginalConfirmation> confirmation_;
    std::vector<size_t> recipients_;
    size_t recipient_ = 0, recipient_first_ = 0;
    void recipient_up();
    void recipient_down();
    bool equip_selected_ = false;
    s2ne::Result<std::vector<unsigned>> try_offers(const OriginalSave::State& state) const;
    std::vector<unsigned> offers(const OriginalSave::State& state) const;
    unsigned unit_price(const OriginalSave::State& state, unsigned id) const;
    s2ne::Result<bool> try_sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, OriginalZeroRun zero) const;
    void sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, OriginalZeroRun zero) const;
    s2ne::Result<bool> try_merchant(std::vector<uint8_t>& image, unsigned portrait = 0) const;

    void merchant(std::vector<uint8_t>& image, unsigned portrait = 0) const;
    void text(std::vector<uint8_t>& image, const std::vector<uint8_t>& value, int x, int y, unsigned color = 42) const;
    void draw_help(std::vector<uint8_t>& image, const std::vector<uint8_t>& value) const;
    void number(std::vector<uint8_t>& image, uint32_t value, int x, int y, unsigned color = 42) const;

    OriginalShopMenu(const std::filesystem::path& root, OriginalShop shop, OriginalMenu font, OriginalInterbattle background, SharedBytes cel, SharedBytes pal);
    s2ne::Result<bool> initialize(const std::filesystem::path& root, OriginalDisc disc);

public:
    explicit OriginalShopMenu(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one);
    static s2ne::Result<std::unique_ptr<OriginalShopMenu>> create(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one);

    std::vector<uint8_t> service_panel(unsigned portrait, unsigned title, uint32_t cash, const OriginalInterbattle& background, bool show_cash = true) const;
    s2ne::Result<bool> try_role_portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const;

    void role_portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const;
    void role_bitmap(std::vector<uint8_t>& image, const std::vector<uint8_t>& pixels, int w, int h, int x, int y) const;
    int panel_width(unsigned id) const;
    int panel_height(unsigned id) const;
    void panel(std::vector<uint8_t>& image, unsigned id, int x, int y) const;
    void shadow(std::vector<uint8_t>& image, unsigned id, int x, int y) const;
    void shared_panel(std::vector<uint8_t>& image, unsigned address, int x, int y) const;
    void outline(std::vector<uint8_t>& image, int left, int top, int right, int bottom, unsigned ink) const;
    Page page() const;
    int selected() const;
    const std::vector<unsigned int>& listed_items() const;
    unsigned listed_price(const OriginalSave::State& state, unsigned id) const;
    bool select_category(unsigned index, OriginalSave::State& state);
    bool return_parent();
    unsigned quantity() const;
    bool select_item(unsigned id, OriginalSave::State& state);
    bool commit_quantity(OriginalSave::State& state);
    unsigned item() const;
    void enable_presentation_timing();
    bool transitioning() const;
    unsigned fade_step() const;
    void frame_presented();
    static unsigned wall_clock_cs();
    static unsigned centiseconds(uint64_t milliseconds);
    static bool timer_pending(unsigned then, unsigned now, unsigned delay);
    std::optional<std::pair<int, int>> take_cursor();
    void release_scroll();
    void clock(unsigned now);
    void tick(unsigned now, OriginalSave::State& state);
    static constexpr unsigned shared_sprite(unsigned address) { return (address - 0x4F7060u) / 4u; }
    size_t recipient_first() const;
    size_t recipient_index() const;
    static uint32_t money(const OriginalSave::State& state);
    bool scan(unsigned code, OriginalSave::State& state);
    bool mouse(int x, int y, bool left, bool right, OriginalSave::State& state);
    std::vector<uint8_t> draw(const OriginalSave::State& state, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
