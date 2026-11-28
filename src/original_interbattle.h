#pragma once
#include "original_menu.h"
#include "title_image.h"
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>

namespace s2ne
{
class OriginalInterbattle
{
    OriginalMenu font_;
    std::vector<uint8_t> background_, caption_palette_;
    std::vector<uint8_t> wide_background_;
    int selected_{ 1 }, pointer_x_{ 298 }, pointer_y_{ 253 };
    bool pending_{}, closed_{}, services_allowed_{ true };
    void caption(std::vector<uint8_t>& image, unsigned id, int x, int y) const;

public:
    struct Rect
    {
        int left, top, right, bottom;
    };
    struct Request
    {
        unsigned option{}, label{}, address{};
    };
    enum class Result
    {
        unsupported,
        returned,
        enter_battle,
        exit_to_title
    };
    // 4EA600 begins with screen title88. Four options use subsequent entries.
    static constexpr std::array<unsigned, 4> labels{ 89, 90, 91, 92 };
    static constexpr std::array<unsigned, 4> targets{ 0x410EEE, 0x410DC0, 0x41115D, 0x416333 };
    static constexpr std::array<Rect, 4> rectangles{ { { 226, 211, 370, 296 }, { 97, 277, 206, 366 }, { 421, 272, 584, 402 }, { 79, 419, 296, 479 } } };
    struct InitializationTag
    {
    };
    OriginalInterbattle(OriginalMenu font, InitializationTag);
    s2ne::Result<bool> initialize(const std::filesystem::path& root, OriginalDisc disc, unsigned resource);
    explicit OriginalInterbattle(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one, unsigned resource = 12);
    static s2ne::Result<std::unique_ptr<OriginalInterbattle>> create(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one, unsigned resource = 12);

    const std::vector<uint8_t>& background() const;
    static bool town_services_allowed(const std::filesystem::path& root, unsigned town);
    static s2ne::Result<unsigned> try_interbattle_town(const std::filesystem::path& root, unsigned town);
    s2ne::Result<bool> try_configure_town(const std::filesystem::path& root, unsigned town);
    bool configure_town(const std::filesystem::path& root, unsigned town);
    bool services_allowed() const;
    void draw_caption(std::vector<uint8_t>& image, unsigned id, int x, int y) const;
    int selected() const;
    bool closed() const;
    bool pending() const;
    static int hit(int x, int y);
    void mouse(int x, int y, bool left_click);
    bool select_option(unsigned index);
    void scan(unsigned code);
    std::optional<Request> request() const;
    // Execute the original subpage via adapter. Unsupported leaves hub pending;
    // it must never silently grant lodging, items, promotion or skip deployment.
    bool dispatch(const std::function<Result(Request)>& adapter);
    std::vector<uint8_t> draw(bool all_captions = false) const;
};
}    //namespace s2ne
