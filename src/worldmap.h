#pragma once
#include "menu.h"
#include "palette.h"
#include "save.h"
#include "stage_route.h"
#include "title_image.h"
#include <algorithm>
namespace s2ne
{
class Worldmap
{
    struct Icon
    {
        unsigned width{}, height{};
        std::vector<uint8_t> pixels;
    };
    std::vector<uint8_t> pixels_, palette_, towns_;
    std::array<uint8_t, 256> briefing_shades_{};
    std::array<uint8_t, 3> briefing_target_{};
    SharedBytes external_rgba_;
    std::array<Icon, 2> icons_;
    unsigned width_{}, height_{}, current_{}, destination_{}, hovered_{};
    int x_{}, y_{};
    bool closed_{};
    int pointer_x_{ 320 }, pointer_y_{ 240 };
    std::optional<std::pair<int, int>> pending_scroll_;
    unsigned last_scroll_{}, delay_{};
    bool waiting_{};
    bool presented_{};
    bool presentation_{}, entering_{}, leaving_{};
    bool travelling_{}, travel_done_{}, travel_presented_{};
    unsigned travel_elapsed_{};
    unsigned arrival_frames_{};
    int travel_x_{}, travel_y_{}, target_x_{}, target_y_{}, start_x_{}, start_y_{};
    std::vector<std::pair<int, int>> trail_;
    unsigned fade_{ 63 };
    bool transition() const;
    uint8_t channel(unsigned ink, unsigned component) const;
    void close();
    static unsigned word(const auto& b, size_t p) { return unsigned(b[p]) | (unsigned(b[p + 1]) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    void center(int x, int y);
    void request_scroll(int dx, int dy);
    void scroll(int dx, int dy);

public:
    void begin_travel();
    bool travel_done() const;
    std::array<uint8_t, 3> text_color(unsigned ink) const;
    void travel_presented();
    void tick_travel(unsigned milliseconds);
    static Result<Worldmap> create(const std::filesystem::path& root, Disc domain, std::vector<uint8_t> towns, const Save::State& state);
    void enable_presentation_timing();
    void frame_presented();
    unsigned fade_level() const;
    void tick(unsigned now);
    bool closed() const;
    unsigned hovered() const;
    unsigned scroll_delay() const;
    std::pair<int, int> viewport() const;
    void scan(unsigned code);
    void mouse(int x, int y, bool left, bool right);
    std::vector<uint8_t> draw(const Menu& font, bool markers = true) const;
};
}    //namespace s2ne
