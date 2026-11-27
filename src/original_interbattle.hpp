#pragma once
#include "original_menu.hpp"
#include "title_image.hpp"
#include <functional>
#include <memory>
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
    void caption(std::vector<uint8_t>& image, unsigned id, int x, int y) const
    {
        const auto text = font_.text_bytes(id);
        // Font helper uses global colors; the PCX menu owns its palette.
        for (unsigned ink : { 2u, 3u })
        {
            std::vector<uint8_t> mask(image.size());
            int delta = ink == 2 ? 1 : 0;
            font_.draw_small(mask, text, x + delta, y + delta, ink);
            for (size_t p = 0; p < mask.size(); p += 4)
            {
                if (mask[p + 3])
                {
                    for (unsigned k = 0; k < 3; ++k)
                    {
                        const unsigned color = opening_palette_channel(uint8_t(caption_palette_[ink * 3 + k] >> 2));
                        const unsigned alpha = mask[p + 3];
                        image[p + k] = uint8_t((color * alpha + unsigned(image[p + k]) * (255 - alpha) + 127) / 255);
                    }
                    image[p + 3] = 255;
                }
            }
        }
    }

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
    OriginalInterbattle(OriginalMenu font, InitializationTag) : font_(std::move(font)) {}
    s2ne::Result<bool> initialize(const std::filesystem::path& root, OriginalDisc disc, unsigned resource)
    {
        if (resource != 12 && resource != 14 && resource != 15 && resource != 13)
        {
            return s2ne::Result<bool>::failure(make_error("Unsupported original interbattle background"));
        }
        auto dat_result = OriginalDiscResources(root).try_dat_entry(disc, 12);
        if (!dat_result)
        {
            return s2ne::Result<bool>::failure(dat_result.error());
        }
        auto dat = std::move(dat_result).value();
        if (dat.size() < 769 || dat[dat.size() - 769] != 12) { return s2ne::Result<bool>::failure(make_error("Missing hub caption palette")); }
        caption_palette_.assign(dat.end() - 768, dat.end());
        auto external = texture_resources.load_image_scaled("interbattle", resource, screen_width, screen_height);
        if (!external) { return s2ne::Result<bool>::failure(external.error()); }
        wide_background_ = external.value().pixels;
        set_fixed_page_background(wide_background_);
        background_ = fixed_page_center();
        return s2ne::Result<bool>::success(true);
    }
    explicit OriginalInterbattle(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one, unsigned resource = 12) : font_(root)
    {
        auto result = initialize(root, disc, resource);
        if (!result)
        {
            fatal(result.error());
        }
    }
    static s2ne::Result<std::unique_ptr<OriginalInterbattle>> create(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one, unsigned resource = 12)
    {
        auto font = OriginalMenu::create(root);
        if (!font) { return s2ne::Result<std::unique_ptr<OriginalInterbattle>>::failure(font.error()); }
        auto result = std::make_unique<OriginalInterbattle>(std::move(font).value(), InitializationTag{});
        auto ready = result->initialize(root, disc, resource);
        if (!ready)
        {
            return s2ne::Result<std::unique_ptr<OriginalInterbattle>>::failure(ready.error());
        }
        return s2ne::Result<std::unique_ptr<OriginalInterbattle>>::success(std::move(result));
    }

    const std::vector<uint8_t>& background() const
    {
        if (!wide_background_.empty()) { fixed_page_background = wide_background_; }
        return background_;
    }
    static bool town_services_allowed(const std::filesystem::path& root, unsigned town)
    {
        auto pal = read_shared_resource(root / "SUPERSP.PAL").value();
        auto bin = read_shared_resource(root / "SUPERSP.BIN").value();
        size_t table = dword(pal, 872) - 14;
        if (!town || town > word(bin, table)) { return false; }
        return bin.at(table + 2 + (town - 1) * 14 + 10) == 1;
    }
    void configure_town(const std::filesystem::path& root, unsigned town)
    {
        services_allowed_ = town_services_allowed(root, town);
        if (!services_allowed_)
        {
            selected_ = 4;
            pending_ = true;
        }
    }
    bool services_allowed() const { return services_allowed_; }
    void draw_caption(std::vector<uint8_t>& image, unsigned id, int x, int y) const { caption(image, id, x, y); }
    int selected() const { return selected_; }
    bool closed() const { return closed_; }
    bool pending() const { return pending_; }
    static int hit(int x, int y)
    {
        for (unsigned i = 0; i < rectangles.size(); ++i)
        {
            auto r = rectangles[i];
            if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom)
            {
                return int(i) + 1;
            }
        }
        return 0;
    }
    void mouse(int x, int y, bool left_click)
    {
        if (closed_ || pending_ || !services_allowed_)
        {
            return;
        }
        pointer_x_ = x;
        pointer_y_ = y;
        int row = hit(x, y);
        selected_ = row;
        if (row)
        {
            if (left_click)
            {
                pending_ = true;
            }
        }
    }
    bool select_option(unsigned index)
    {
        if (closed_ || pending_ || index >= labels.size() || (!services_allowed_ && index != 3)) { return false; }
        selected_ = int(index + 1);
        pending_ = true;
        return true;
    }
    void scan(unsigned code)
    {
        if (closed_ || pending_ || !services_allowed_)
        {
            return;
        }
        // 410A3F: Left AND Right AND Tab all advance; Escape is disabled at hub.
        if (code == 0x4B || code == 0x4D || code == 0x0F)
        {
            selected_ = selected_ % 4 + 1;
            auto r = rectangles[size_t(selected_ - 1)];
            pointer_x_ = (r.left + r.right) / 2;
            pointer_y_ = (r.top + r.bottom) / 2;
        }
        else if ((code == 0x1C || code == 0x39) && selected_ > 0)
        {
            pending_ = true;
        }
    }
    std::optional<Request> request() const
    {
        if (!pending_ || closed_)
        {
            return std::nullopt;
        }
        size_t row = size_t(selected_ - 1);
        return Request{ unsigned(selected_), labels[row], targets[row] };
    }
    // Execute the original subpage via adapter. Unsupported leaves hub pending;
    // it must never silently grant lodging, items, promotion or skip deployment.
    bool dispatch(const std::function<Result(Request)>& adapter)
    {
        auto req = request();
        if (!req || !adapter)
        {
            return false;
        }
        Result result = adapter(*req);
        if (result == Result::unsupported)
        {
            return false;
        }
        if (result == Result::enter_battle && req->option != 4)
        {
            return false;
        }
        pending_ = false;
        closed_ = result == Result::enter_battle || result == Result::exit_to_title;
        return true;
    }
    std::vector<uint8_t> draw(bool all_captions = false) const
    {
        auto image = background();
        if (all_captions)
        {
            for (size_t i = 0; i < labels.size(); ++i)
            {
                if (i < 3 && !services_allowed_) { continue; }
                auto r = rectangles[i];
                auto t = font_.text_bytes(labels[i]);
                int x = std::clamp((r.left + r.right) / 2 - int(t.size()) * 4, 0, 639 - int(t.size()) * 8);
                caption(image, labels[i], x, (r.top + r.bottom) / 2);
            }
        }
        else if (selected_ > 0 && (selected_ == 4 || services_allowed_))
        {
            auto text = font_.text_bytes(labels[size_t(selected_ - 1)]);
            int x = std::clamp(pointer_x_ - int(text.size()) * 4, 0, 639 - int(text.size()) * 8);
            caption(image, labels[size_t(selected_ - 1)], x, std::max(30, pointer_y_ - 30));
        }
        return image;
    }
};
}    //namespace s2ne
