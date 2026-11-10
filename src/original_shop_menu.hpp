#pragma once
#include "original_confirmation.hpp"
#include "original_equipment.hpp"
#include "original_interbattle.hpp"
#include "original_shop.hpp"
#include <chrono>
#include <ctime>
#include <memory>
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
    std::vector<uint8_t> cel_, pal_;
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
    void begin_entry()
    {
        if (presentation_driven_)
        {
            fade_step_ = 0;
        }
        cached_first_ = -1;
    }
    std::vector<uint8_t> fade(std::vector<uint8_t> image) const
    {
        if (!presentation_driven_)
        {
            return image;
        }
        unsigned intensity = std::min(fade_step_, 15u) * 16;
        // 472EF1/47F5D2: (six-bit DAC component<<2)*(step<<2)/255.
        for (size_t p = 0; p < image.size(); p += 4)
        {
            for (unsigned c = 0; c < 3; ++c)
            {
                unsigned dac = (unsigned(image[p + c]) * 63 + 254) / 255;
                image[p + c] = uint8_t((dac * 4) * intensity / 255);
            }
        }
        return image;
    }
    std::vector<uint8_t> towns_;
    std::unique_ptr<OriginalEquipment> equipment_;
    std::optional<OriginalConfirmation> confirmation_;
    std::vector<size_t> recipients_;
    size_t recipient_ = 0, recipient_first_ = 0;
    void recipient_up()
    {
        if (recipient_ > recipient_first_)
        {
            --recipient_;
        }
        else if (recipient_first_)
        {
            --recipient_first_;
            --recipient_;
        }
    }
    void recipient_down()
    {
        if (recipient_ + 1 >= recipients_.size())
        {
            return;
        }
        ++recipient_;
        if (recipient_ - recipient_first_ >= 5)
        {
            ++recipient_first_;
        }
    }
    bool equip_selected_ = false;
    s2ne::Result<std::vector<unsigned>> try_offers(const OriginalSave::State& state) const
    {
        // 411288 indexes town4F7CE8, not the battle stage4F7CD8.
        unsigned town = unsigned(state.globals_4f7cd0[24]) | (unsigned(state.globals_4f7cd0[25]) << 8);
        if (!town || town > word(towns_, 0) || option_ < 1 || option_ > 3)
        {
            return s2ne::Result<std::vector<unsigned>>::success({ });
        }
        size_t record = 2 + size_t(town - 1) * 14;
        unsigned count = towns_.at(record + 10 + option_);
        size_t p = dword(towns_, record);
        while (towns_.at(p))
        {
            ++p;
        }
        ++p;
        for (int category = 1; category < option_; ++category)
        {
            p += 2u * towns_.at(record + 10 + category);
        }
        std::vector<unsigned> result;
        for (unsigned i = 0; i < count; ++i)
        {
            unsigned id = word(towns_, p + 2 * i);
            if (!id || id > state.groups[0].inventory.size())
            {
                return s2ne::Result<std::vector<unsigned>>::failure(make_error("Unsupported original town offer ID"));
            }
            result.push_back(id);
        }
        return s2ne::Result<std::vector<unsigned>>::success(result);
    }
    std::vector<unsigned> offers(const OriginalSave::State& state) const
    {
        auto result = try_offers(state);
        return std::move(result).value();
    }
    unsigned unit_price(const OriginalSave::State& state, unsigned id) const { return buying_ ? shop_.buy_price(state, id) : shop_.sale_price(state, id); }
    static s2ne::Result<std::vector<uint8_t>> try_read(const std::filesystem::path& path) { return read_resource(path); }
    static std::vector<uint8_t> read(const std::filesystem::path& path) { return try_read(path).value(); }
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    s2ne::Result<bool> try_sprite(std::vector<uint8_t>& image, unsigned id, int left, int top) const
    {
        size_t g = dword(pal_, 816) - 57, p = g + dword(cel_, g + 2 + (id - 142) * 8);
        unsigned w = word(cel_, p) + 1, h = word(cel_, p + 2) + 1;
        p += 4;
        for (unsigned y = 0; y < h; ++y)
        {
            for (unsigned x = 0; x < w;)
            {
                unsigned op = cel_.at(p++), n = op ? op & 127 : 1, c = 0;
                if (!n || n > w - x)
                {
                    return s2ne::Result<bool>::failure(make_error("Invalid original shop sprite"));
                }
                if (op && !(op & 128))
                {
                    c = cel_.at(p++);
                }
                while (n--)
                {
                    unsigned ink = op & 128 ? cel_.at(p++) : c;
                    int xx = left + int(x++), yy = top + int(y);
                    if (ink && xx >= 0 && xx < 640 && yy >= 0 && yy < 480)
                    {
                        size_t q = (size_t(yy) * 640 + xx) * 4;
                        for (unsigned k = 0; k < 3; ++k)
                        {
                            image.at(q + k) = uint8_t(unsigned(pal_.at(ink * 3 + k)) * 255 / 63);
                        }
                        image.at(q + 3) = 255;
                    }
                }
            }
        }

        return s2ne::Result<bool>::success(true);
    }
    void sprite(std::vector<uint8_t>& image, unsigned id, int left, int top) const
    {
        auto result = try_sprite(image, id, left, top);
        if (!result)
        {
            report_error(result.error());
        }
    }
    s2ne::Result<bool> try_merchant(std::vector<uint8_t>& image, unsigned portrait = 0) const
    {
        // 41144E ->426728(167,0): CEL group3 portrait pair, index332.
        size_t g = dword(pal_, 824) - 58, p = g + dword(cel_, g + 2 + (portrait ? portrait * 2 - 2 : buying_ ? 86 :
                                                                                                               332)
                                                          * 8);
        unsigned type = cel_.at(p++), w = word(cel_, p) + 1, h = word(cel_, p + 2) + 1;
        p += 4;
        if (type > 1 || w > 640 || h > 480)
        {
            return s2ne::Result<bool>::failure(make_error("Original merchant portrait framing"));
        }
        int left = 80 - int(w) / 2;
        for (unsigned y = 0; y < h; ++y)
        {
            for (unsigned x = 0; x < w;)
            {
                unsigned op = type ? 0x81 : cel_.at(p++), n = op ? op & 127 : 1, c = 0;
                if (!n || n > w - x)
                {
                    return s2ne::Result<bool>::failure(make_error("Original merchant portrait run"));
                }
                if (op && !(op & 128))
                {
                    c = cel_.at(p++);
                }
                while (n--)
                {
                    unsigned ink = op & 128 ? cel_.at(p++) : c;
                    int xx = left + int(x++), yy = 37 + int(y);
                    if (ink && xx >= 0 && xx < 640 && yy < 480)
                    {
                        size_t q = (size_t(yy) * 640 + xx) * 4;
                        for (unsigned k = 0; k < 3; ++k)
                        {
                            image.at(q + k) = uint8_t(unsigned(pal_.at(ink * 3 + k)) * 255 / 63);
                        }
                        image.at(q + 3) = 255;
                    }
                }
            }
        }

        return s2ne::Result<bool>::success(true);
    }
    void merchant(std::vector<uint8_t>& image, unsigned portrait = 0) const
    {
        auto result = try_merchant(image, portrait);
        if (!result)
        {
            report_error(result.error());
        }
    }
    void text(std::vector<uint8_t>& image, const std::vector<uint8_t>& value, int x, int y, unsigned color = 42) const { font_.draw_original_large(image, value, x, y, color); }
    void draw_help(std::vector<uint8_t>& image, const std::vector<uint8_t>& value) const
    {
        size_t p = 0;
        for (int row = 0; row < 8; ++row)
        {
            int compensation = 0;
            for (int cell = 0, units = 1; units < 18; ++cell)
            {
                if (p == value.size() || !value[p] || value[p] == '\n')
                {
                    return;
                }
                unsigned lead = value[p++];
                if (lead == '*')
                {
                    if (!cell)
                    {
                        --cell;
                        continue;
                    }
                    break;
                }
                std::vector<uint8_t> glyph{ uint8_t(std::max(lead, 30u)) };
                if (lead > 128)
                {
                    if (p == value.size()) { return; }
                    glyph.push_back(value[p++]);
                    units += 2;
                }
                else
                {
                    ++units;
                }
                font_.draw_small(image, glyph, 29 + cell * 16 - compensation, 302 + row * 16, 42, true);
                if (lead <= 128) { compensation += 8; }
            }
        }
    }
    void number(std::vector<uint8_t>& image, uint32_t value, int x, int y, unsigned color = 42) const
    {
        auto s = std::to_string(value);
        text(image, { s.begin(), s.end() }, x, y, color);
    }

public:
    struct InitializationTag
    {
    };
    OriginalShopMenu(const std::filesystem::path& root, OriginalDisc disc, InitializationTag) : root_(root), shop_(root), font_(root), background_(root, disc, 14), cel_(read(root / "SUPERSP.CEL")), pal_(read(root / "SUPERSP.PAL")) { }
    s2ne::Result<bool> initialize(const std::filesystem::path& root, OriginalDisc disc)
    {
        auto bin = read(root / "SUPERSP.BIN");
        size_t town = dword(pal_, 848 + 3 * 8) - 14, size = dword(pal_, 852 + 3 * 8) - 14;
        if (town > bin.size() || size > bin.size() - town)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid original town BIN4"));
        }
        towns_.assign(bin.begin() + town, bin.begin() + town + size);
        auto bytes = OriginalDiscResources(root).dat_entry(disc, 3);
        size_t p = 2;
        unsigned count = word(bytes, 0);
        for (unsigned i = 0; i < count; ++i)
        {
            size_t start = p;
            while (bytes.at(p))
            {
                ++p;
            }
            help_.emplace_back(bytes.begin() + start, bytes.begin() + p);
            ++p;
        }

        return s2ne::Result<bool>::success(true);
    }
    explicit OriginalShopMenu(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one) : root_(root), shop_(root), font_(root), background_(root, disc, 14), cel_(read(root / "SUPERSP.CEL")), pal_(read(root / "SUPERSP.PAL"))
    {
        auto result = initialize(root, disc);
        if (!result)
        {
            fatal(result.error());
        }
    }
    static s2ne::Result<std::unique_ptr<OriginalShopMenu>> create(const std::filesystem::path& root, OriginalDisc disc = OriginalDisc::one)
    {
        auto result = std::make_unique<OriginalShopMenu>(root, disc, InitializationTag{ });
        auto ready = result->initialize(root, disc);
        if (!ready)
        {
            return s2ne::Result<std::unique_ptr<OriginalShopMenu>>::failure(ready.error());
        }
        return s2ne::Result<std::unique_ptr<OriginalShopMenu>>::success(std::move(result));
    }

    std::vector<uint8_t> service_panel(unsigned portrait, unsigned title, uint32_t cash, const OriginalInterbattle& background, bool show_cash = true) const
    {
        auto image = background.indexed_background(pal_);
        sprite(image, 235, 6, 8);
        if (show_cash)
        {
            sprite(image, 212, 352, 200);
        }
        merchant(image, portrait);
        auto caption = font_.text_bytes(title);
        text(image, caption, 80 - int(caption.size()) * 6, 176, 54);
        if (show_cash)
        {
            text(image, font_.text_bytes(205), 355, 205, 179);
            number(image, cash, 480, 205, 179);
        }
        return image;
    }
    s2ne::Result<bool> try_role_portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const
    {
        size_t g = dword(pal_, 824) - 58, p = g + dword(cel_, g + 2 + (id * 2 - 2) * 8);
        unsigned type = cel_.at(p++), w = word(cel_, p) + 1, h = word(cel_, p + 2) + 1;
        p += 4;
        for (unsigned yy = 0; yy < h; ++yy)
        {
            for (unsigned xx = 0; xx < w;)
            {
                unsigned op = type ? 0x81 : cel_.at(p++), n = op ? op & 127 : 1, c = 0;
                if (!n || n > w - xx)
                {
                    return s2ne::Result<bool>::failure(make_error("Role portrait run"));
                }
                if (op && !(op & 128))
                {
                    c = cel_.at(p++);
                }
                while (n--)
                {
                    unsigned ink = op & 128 ? cel_.at(p++) : c;
                    int dx = x + int(xx++), dy = y + int(yy);
                    if (ink && dx >= 0 && dx < 640 && dy >= 0 && dy < 480)
                    {
                        size_t q = (size_t(dy) * 640 + dx) * 4;
                        for (unsigned k = 0; k < 3; ++k)
                        {
                            image.at(q + k) = uint8_t(unsigned(pal_.at(ink * 3 + k)) * 255 / 63);
                        }
                        image.at(q + 3) = 255;
                    }
                }
            }
        }

        return s2ne::Result<bool>::success(true);
    }
    void role_portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const
    {
        auto result = try_role_portrait(image, id, x, y);
        if (!result)
        {
            report_error(result.error());
        }
    }
    void role_bitmap(std::vector<uint8_t>& image, const std::vector<uint8_t>& pixels, int w, int h, int x, int y) const
    {
        for (int yy = 0; yy < h; ++yy)
        {
            for (int xx = 0; xx < w; ++xx)
            {
                unsigned ink = pixels.at(size_t(yy) * w + xx);
                int dx = x + xx, dy = y + yy;
                if (!ink || dx < 0 || dx >= 640 || dy < 0 || dy >= 480)
                {
                    continue;
                }
                size_t q = (size_t(dy) * 640 + dx) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    image.at(q + k) = uint8_t(unsigned(pal_.at(ink * 3 + k)) * 255 / 63);
                }
                image.at(q + 3) = 255;
            }
        }
    }
    std::vector<uint8_t> indexed_scene(const OriginalInterbattle& scene) const { return scene.indexed_background(pal_); }
    int panel_height(unsigned id) const
    {
        size_t g = dword(pal_, 816) - 57, p = g + dword(cel_, g + 2 + (id - 142) * 8);
        return int(word(cel_, p + 2)) + 1;
    }
    void panel(std::vector<uint8_t>& image, unsigned id, int x, int y) const { sprite(image, id, x, y); }
    void shared_panel(std::vector<uint8_t>& image, unsigned address, int x, int y) const { sprite(image, shared_sprite(address), x, y); }
    void outline(std::vector<uint8_t>& image, int left, int top, int right, int bottom, unsigned ink) const
    {
        for (int y = top; y <= bottom; ++y)
        {
            for (int x = left; x <= right; ++x)
            {
                if ((x != left && x != right && y != top && y != bottom) || x < 0 || x >= 640 || y < 0 || y >= 480) { continue; }
                size_t p = (size_t(y) * 640 + x) * 4;
                for (unsigned k = 0; k < 3; ++k) { image.at(p + k) = uint8_t(unsigned(pal_.at(ink * 3 + k)) * 255 / 63); }
                image.at(p + 3) = 255;
            }
        }
    }
    Page page() const { return page_; }
    int selected() const { return option_; }
    unsigned quantity() const { return quantity_; }
    unsigned item() const { return items_.empty() ? 0 : items_.at(size_t(first_ + row_)); }
    void enable_presentation_timing() { presentation_driven_ = true; }
    bool transitioning() const { return fade_step_ < 16; }
    unsigned fade_step() const { return fade_step_; }
    void frame_presented()
    {
        if (fade_step_ < 16)
        {
            ++fade_step_;
        }
    }
    static unsigned wall_clock_cs()
    {
        auto now = std::chrono::system_clock::now();
        auto seconds = std::chrono::system_clock::to_time_t(now);
        std::tm local{ };
        localtime_s(&local, &seconds);
        auto fraction = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
        return unsigned((local.tm_min * 60 + local.tm_sec) * 100 + fraction / 10);
    }
    static unsigned centiseconds(uint64_t milliseconds) { return unsigned((milliseconds / 10) % 360000); }
    static bool timer_pending(unsigned then, unsigned now, unsigned delay) { return std::abs(int(then) - int(now)) < int(delay); }
    std::optional<std::pair<int, int>> take_cursor()
    {
        auto result = cursor_request_;
        cursor_request_.reset();
        return result;
    }
    void release_scroll() { held_scroll_ = 0; }
    void clock(unsigned now) { clock_cs_ = now; }
    void tick(unsigned now, OriginalSave::State& state)
    {
        clock_cs_ = now;
        if (page_ != Page::sale)
        {
            held_scroll_ = 0;
            return;
        }
        if (held_scroll_ && !timer_pending(held_stamp_, now, 10))
        {
            scan(held_scroll_, state);
            cursor_request_.reset();
            held_stamp_ = now;
        }
    }
    static constexpr unsigned shared_sprite(unsigned address) { return (address - 0x4F7060u) / 4u - 1u; }
    size_t recipient_first() const { return recipient_first_; }
    size_t recipient_index() const { return recipient_; }
    static uint32_t money(const OriginalSave::State& state)
    {
        uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            v |= uint32_t(state.groups[0].metadata[i]) << (i * 8);
        }
        return v;
    }
    bool scan(unsigned code, OriginalSave::State& state)
    {
        struct RestoreCursor
        {
            OriginalShopMenu& menu;
            Page before;
            ~RestoreCursor()
            {
                if (before != Page::sale && menu.page_ == Page::sale)
                {
                    menu.cursor_request_ = std::pair{ 224 + 54 * menu.row_, 356 };
                    if (before == Page::shop)
                    {
                        menu.begin_entry();
                    }
                }
                if (menu.page_ != Page::sale)
                {
                    menu.held_scroll_ = 0;
                }
            }
        } restore{ *this, page_ };
        if (transitioning() || page_ == Page::closed)
        {
            return false;
        }
        if (insufficient_)
        {
            if (code)
            {
                insufficient_ = false;
            }
            return false;
        }
        if (page_ == Page::equipment_confirm)
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
                page_ = Page::equipment;
                if (*result)
                {
                    auto staged = state;
                    if (!shop_.buy(staged, item(), 1))
                    {
                        return false;
                    }
                    if (equip_selected_ && !equipment_->equip(staged, recipients_.at(recipient_), item()))
                    {
                        return false;
                    }
                    state = std::move(staged);
                    page_ = Page::sale;
                    return true;
                }
            }
            return false;
        }
        if (page_ == Page::equipment)
        {
            if (code == 1)
            {
                page_ = Page::sale;
                return false;
            }
            if (code == 0x48)
            {
                recipient_up();
            }
            if (code == 0x50)
            {
                recipient_down();
            }
            if (code == 0x0F)
            {
                equip_selected_ = !equip_selected_ && !recipients_.empty();
            }
            if (code == 0x1C || code == 0x39)
            {
                confirmation_.emplace(root_, font_, equip_selected_ ? 207 : 96, 212, 228, true);
                page_ = Page::equipment_confirm;
            }
            return false;
        }
        if (code == 1)
        {
            page_ = page_ == Page::quantity ? Page::sale : page_ == Page::sale ? Page::shop :
                                                                                 Page::closed;
            return false;
        }
        if (page_ == Page::shop)
        {
            if (code == 0x4B || code == 0x4D || code == 0x0F)
            {
                option_ = option_ % 4 + 1;
            }
            if (code == 0x1C || code == 0x39)
            {
                buying_ = option_ != 4;
                auto items = buying_ ? offers(state) : shop_.sale_items(state);
                if (!buying_ || !items.empty())
                {
                    items_ = std::move(items);
                    first_ = row_ = 0;
                    page_ = Page::sale;
                }
            }
        }
        else if (page_ == Page::sale)
        {
            if (code == 0x4B)
            {
                if (row_ > 0)
                {
                    --row_;
                }
                else if (first_ > 0)
                {
                    --first_;
                }
            }
            if (code == 0x4D && size_t(first_ + row_ + 1) < items_.size())
            {
                if (row_ < 7)
                {
                    ++row_;
                }
                else
                {
                    ++first_;
                }
            }
            if ((code == 0x1C || code == 0x39) && item() && buying_ && unit_price(state, item()) > money(state))
            {
                insufficient_ = true;
                return false;
            }
            if ((code == 0x1C || code == 0x39) && item() && (!buying_ || unit_price(state, item()) <= money(state)))
            {
                quantity_ = 1;
                if (buying_ && option_ <= 2)
                {
                    if (!equipment_)
                    {
                        equipment_ = std::make_unique<OriginalEquipment>(root_);
                    }
                    recipients_ = equipment_->recipients(state, item());
                    recipient_ = recipient_first_ = 0;
                    equip_selected_ = false;
                    page_ = Page::equipment;
                }
                else
                {
                    page_ = Page::quantity;
                }
            }
        }
        else if (page_ == Page::quantity)
        {
            if (code == 0x4B && quantity_ > 1)
            {
                --quantity_;
            }
            if (code == 0x4D && (!buying_ ? quantity_ < state.groups[0].inventory.at(item() - 1) : uint64_t(quantity_ + 1) * unit_price(state, item()) <= money(state)))
            {
                ++quantity_;
            }
            if ((code == 0x1C || (buying_ && code == 0x39)) && (buying_ ? shop_.buy(state, item(), quantity_) : shop_.sell(state, item(), quantity_)))
            {
                items_ = buying_ ? offers(state) : shop_.sale_items(state);
                first_ = std::max(0, std::min(first_, int(items_.size()) - 8));
                row_ = std::max(0, std::min(row_, int(items_.size()) - first_ - 1));
                page_ = Page::sale;
                return true;
            }
        }
        if (page_ == Page::sale && (code == 0x4B || code == 0x4D))
        {
            cursor_request_ = std::pair{ 224 + 54 * row_, 356 };
        }
        return false;
    }
    bool mouse(int x, int y, bool left, bool right, OriginalSave::State& state)
    {
        if (transitioning())
        {
            return false;
        }
        if (insufficient_)
        {
            if (left || right)
            {
                insufficient_ = false;
            }
            return false;
        }
        if (right)
        {
            return scan(1, state);
        }
        if (page_ == Page::equipment_confirm)
        {
            confirmation_->mouse(float(x), float(y), left);
            return scan(0, state);
        }
        if (page_ == Page::equipment)
        {
            int top = 480 - panel_height(218);
            if (left && x >= 239 && x <= 258 && y >= top + 51 && y <= top + 230 && !recipients_.empty())
            {
                if (y <= top + 74)
                {
                    recipient_up();
                    equip_selected_ = true;
                }
                else if (y >= top + 207)
                {
                    recipient_down();
                    equip_selected_ = true;
                }
                else if (y >= top + 76 && y <= top + 205 && recipients_.size() > 5)
                {
                    size_t first = size_t((y - top - 76) * int(recipients_.size()) / 129);
                    recipient_first_ = std::min(first, recipients_.size() - 5);
                    recipient_ = recipient_first_ + (y >= top + 175 ? 4 : 0);
                    equip_selected_ = true;
                }
                return false;
            }
            if (x >= 262 && x <= 459 && y >= top + 17 && y <= top + 45)
            {
                equip_selected_ = false;
                return left ? scan(0x1C, state) : false;
            }
            if (x >= 269 && x <= 453 && y >= top + 52 && y <= top + 227 && !recipients_.empty())
            {
                size_t first = recipient_first_, n = first + size_t((y - top - 52) / 35);
                if (n < recipients_.size())
                {
                    bool same = equip_selected_ && recipient_ == n;
                    recipient_ = n;
                    equip_selected_ = true;
                    if (same && left)
                    {
                        return scan(0x1C, state);
                    }
                }
            }
            return false;
        }
        if (page_ == Page::shop)
        {
            for (unsigned i = 0; i < 4; ++i)
            {
                auto r = rectangles[i];
                if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom)
                {
                    option_ = int(i) + 1;
                    if (left)
                    {
                        return scan(0x1C, state);
                    }
                }
            }
        }
        else if (page_ == Page::sale)
        {
            if (x >= 196 && x < 628 && y >= 307 && y <= 379)
            {
                int row = (x - 196) / 54;
                if (row < 8 && size_t(first_ + row) < items_.size())
                {
                    row_ = row;
                    return left ? scan(0x1C, state) : false;
                }
            }
            else if (left && x >= 196 && x <= 625 && y >= 382 && y <= 401)
            {
                if (x <= 221 || x >= 600)
                {
                    held_scroll_ = x <= 221 ? 0x4B : 0x4D;
                    held_stamp_ = clock_cs_;
                    auto result = scan(held_scroll_, state);
                    cursor_request_.reset();
                    return result;
                }
                held_scroll_ = 0;
                first_ = std::clamp((x - 222) * int(items_.size()) / 349, 0, std::max(0, int(items_.size()) - 8));
                row_ = x >= 571 ? 7 : 0;
                if (row_ >= int(items_.size()) - 1)
                {
                    row_ = int(items_.size()) - 1;
                    first_ = 0;
                }
            }
        }
        else if (page_ == Page::quantity && left && y >= 413 && y <= 443)
        {
            int base = buying_ ? 258 : 198;
            if (x >= base + 227 && x <= base + 292)
            {
                return scan(0x1C, state);
            }
            if (x >= base + 171 && x <= base + 195)
            {
                return scan(0x4B, state);
            }
            if (x >= base + 200 && x <= base + 225)
            {
                return scan(0x4D, state);
            }
        }
        return false;
    }
    std::vector<uint8_t> draw(const OriginalSave::State& state) const
    {
        auto image = background_.background();
        if (page_ == Page::shop)
        {
            auto r = rectangles[size_t(option_ - 1)];
            auto s = font_.text_bytes(unsigned(option_ == 4 ? 199 : 92 + option_));
            font_.draw_small(image, s, (r.left + r.right) / 2 - int(s.size()) * 4, (r.top + r.bottom) / 2 - 30, 3);
            return image;
        }
        std::vector<uint8_t> key;
        key.reserve(8 + state.groups[0].inventory.size());
        for (unsigned i = 0; i < 4; ++i)
        {
            key.push_back(state.groups[0].metadata[i]);
        }
        key.push_back(uint8_t(option_));
        key.insert(key.end(), state.groups[0].inventory.begin(), state.groups[0].inventory.end());
        bool reuse = buying_ && page_ == Page::sale && cached_first_ == first_ && cached_key_ == key && !cached_panel_.empty();
        if (reuse)
        {
            image = cached_panel_;
        }
        else
        {
            sprite(image, 235, 6, 8);
            sprite(image, 212, 352, 200);
            sprite(image, 211, 4, 236);
            merchant(image);
            if (buying_)
            {
                text(image, font_.text_bytes(100), 149, 42);
            }
            auto title = font_.text_bytes(buying_ ? 198 : 194);
            text(image, title, 80 - int(title.size()) * 6, 176, 54);
            text(image, font_.text_bytes(205), 355, 205, 179);
            if (page_ == Page::sale)
            {
                number(image, money(state), 480, 205, 179);
            }
            text(image, font_.text_bytes(buying_ ? 202 : 203), 410 - int(font_.text_bytes(buying_ ? 202 : 203).size()) * 6, 279, 139);
            for (unsigned col = 0; col < 8 && size_t(first_) + col < items_.size(); ++col)
            {
                unsigned id = items_[size_t(first_) + col];
                sprite(image, id + 492, 204 + int(col) * 54, 313);
                auto price = std::to_string(unit_price(state, id));
                // 411F5B uses 422402 in both buy and sale paths (8-pixel ASCII advance).
                font_.draw_small(image, { price.begin(), price.end() }, 220 - int(price.size()) * 4 + int(col) * 54, 358, buying_ && unit_price(state, id) > money(state) ? 68 : 190, true);
            }
            if (buying_ && page_ == Page::sale)
            {
                cached_first_ = first_;
                cached_key_ = std::move(key);
                cached_panel_ = image;
            }
        }
        if (item())
        {
            int thumb = 222;
            if (first_ + row_ > 0)
            {
                thumb += (first_ + row_ + 1) * 349 / int(items_.size());
            }
            sprite(image, 213, thumb, 382);
            // 413061:475970 rectangle, color60, native selected column bounds.
            int left = 196 + row_ * 54, right = 247 + row_ * 54;
            auto pixel = [&](int x, int y)
            {
                size_t q = (size_t(y) * 640 + x) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    image.at(q + k) = uint8_t(unsigned(pal_.at(60 * 3 + k)) * 255 / 63);
                }
                image.at(q + 3) = 255;
            };
            for (int x = left; x <= right; ++x)
            {
                pixel(x, 307);
                pixel(x, 379);
            }
            for (int y = 307; y <= 379; ++y)
            {
                pixel(left, y);
                pixel(right, y);
            }
            auto name = shop_.name(item());
            text(image, name, 93 - int(name.size()) * 6, 267, 54);
            const auto& help = help_.at(item());
            // 411842 calls 4224F2: fixed small font, nine 16-pixel cells, eight rows.
            draw_help(image, help);
            if (page_ == Page::sale)
            {
                auto owned = font_.text_bytes(buying_ ? 251 : 110);
                text(image, owned, 264, 416);
                number(image, state.groups[0].inventory.at(item() - 1), buying_ ? 274 + int(owned.size()) * 12 : 400, 416);
            }
        }
        if (insufficient_)
        {
            unsigned id = shared_sprite(0x4F72C8);
            size_t g = dword(pal_, 816) - 57, p = g + dword(cel_, g + 2 + (id - 142) * 8);
            sprite(image, id, 268, 382);
            auto message = font_.text_bytes(104);
            text(image, message, 268 + int(word(cel_, p) + 1) / 2 - int(message.size()) * 6, 382 + int(word(cel_, p + 2) + 1) / 2 - 12);
        }
        if (page_ == Page::equipment || page_ == Page::equipment_confirm)
        {
            int top = 480 - panel_height(218);
            sprite(image, 218, 0, top);
            number(image, money(state) - unit_price(state, item()), 480, 205, 179);
            auto prompt = font_.text_bytes(208), name = shop_.name(item());
            for (size_t i = 0; i + 1 < prompt.size(); ++i)
            {
                if (prompt[i] == '%')
                {
                    auto n = std::to_string(unit_price(state, item()));
                    auto v = prompt[i + 1] == 's' ? name : std::vector<uint8_t>(n.begin(), n.end());
                    prompt.erase(prompt.begin() + i, prompt.begin() + i + 2);
                    prompt.insert(prompt.begin() + i, v.begin(), v.end());
                    i += v.size();
                }
            }
            text(image, prompt, 149, 90);
            text(image, font_.text_bytes(204), 277, top + 20, equip_selected_ ? 45 : 242);
            size_t first = recipient_first_;
            for (size_t i = 0; i < 5 && first + i < recipients_.size(); ++i)
            {
                const auto& r = state.groups[0].units.at(recipients_[first + i]);
                std::vector<uint8_t> n;
                for (size_t p = 0x10; p < 0x23 && r.at(p); ++p)
                {
                    n.push_back(r[p]);
                }
                text(image, n, 305, top + 59 + 35 * int(i), equip_selected_ && first + i == recipient_ ? 242 : 52);
            }
            if (!recipients_.empty())
            {
                int thumb = top + 76;
                if (recipient_)
                {
                    thumb += int((recipient_ + 1) * 100 / recipients_.size());
                }
                sprite(image, shared_sprite(0x4F72AC), 239, thumb);
            }
            if (equip_selected_ && !recipients_.empty())
            {
                const auto& r = state.groups[0].units.at(recipients_[recipient_]);
                unsigned old = word(r, 0xCB + 2 * equipment_->slot(item()));
                for (unsigned i = 0; i < 6; ++i)
                {
                    unsigned id = word(r, 0xCB + 2 * i);
                    if (id)
                    {
                        sprite(image, id + 492, 70, top + 20 + 35 * int(i));
                        text(image, shop_.name(id), 104, top + 25 + 35 * int(i), 233);
                    }
                }
                int y = top + 80;
                std::array<unsigned, 7> offsets{ 17, 21, 22, 16, 19, 20, 23 }, labels{ 7, 8, 11, 119, 116, 117, 118 };
                for (unsigned i = 0; i < 7; ++i)
                {
                    int a = equipment_->field(old, offsets[i], i == 0), b = equipment_->field(item(), offsets[i], i == 0);
                    if (a == b)
                    {
                        continue;
                    }
                    font_.draw_small(image, font_.text_bytes(labels[i]), 491, y, 137, true);
                    if (i == 3)
                    {
                        a = unsigned(a) & 255;
                        b = unsigned(b) & 255;
                        a %= 10;
                        b %= 10;
                    }
                    else if (i >= 4)
                    {
                        a = unsigned(a) & 255;
                        b = unsigned(b) & 255;
                    }
                    int value = i == 0 ? word(r, 0xC3) : i == 1 ? word(r, 0xC5) :
                                                                  a;
                    auto n = std::to_string(value), next = std::to_string(i < 2 ? std::max(0, value + b - a) : b);
                    font_.draw_small(image, { n.begin(), n.end() }, 532, y, 109, true);
                    sprite(image, shared_sprite(0x4F72B0), 560, y + 4);
                    font_.draw_small(image, { next.begin(), next.end() }, 588, y, a > b ? 59 : 167, true);
                    y += 24;
                }
                if (y > top + 80)
                {
                    text(image, font_.text_bytes(120), 503, top + 37, 233);
                }
            }
            if (confirmation_)
            {
                confirmation_->draw(image, font_);
            }
            return image;
        }
        if (page_ == Page::quantity)
        {
            sprite(image, 216, buying_ ? 258 : 198, 413);
            auto prompt = font_.text_bytes(buying_ ? 200 : 201), name = shop_.name(item());
            for (size_t i = 0; i + 1 < prompt.size(); ++i)
            {
                if (prompt[i] == '%' && prompt[i + 1] == 's')
                {
                    prompt.erase(prompt.begin() + i, prompt.begin() + i + 2);
                    prompt.insert(prompt.begin() + i, name.begin(), name.end());
                    break;
                }
            }
            text(image, prompt, 149, 90);
            if (!buying_)
            {
                // 413BA0 draws remaining stock only for selling; 411FA1 has no stock field.
                text(image, font_.text_bytes(110), 530, 416);
                number(image, state.groups[0].inventory.at(item() - 1) - quantity_, 540 + int(font_.text_bytes(110).size()) * 12, 416);
            }
            number(image, quantity_, buying_ ? 272 : 212, 415, 59);
            auto amount = std::string("$") + std::to_string(quantity_ * unit_price(state, item()));
            text(image, { amount.begin(), amount.end() }, buying_ ? 328 : 268, 415, 59);
            text(image, font_.text_bytes(206), buying_ ? 488 : 428, 416);
            number(image, buying_ ? money(state) - quantity_ * unit_price(state, item()) : money(state) + quantity_ * unit_price(state, item()), 480, 205, 179);
        }
        return fade(std::move(image));
    }
};
}    //namespace s2ne
