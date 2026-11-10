#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_random.hpp"
#include "original_settings.hpp"
#include <cmath>
#include <functional>
#include <optional>

namespace s2ne
{
// 4409B7 / 45954D / 459FCF. Commit is deliberately a host transaction:
// changing only Unit::appearance would omit 45B5C4's equipment/RNG/save effects.
class OriginalPromotion
{
public:
    static constexpr unsigned eligibility_address = 0x4409B7, selection_address = 0x45954D,
                              confirmation_address = 0x459FCF, commit_address = 0x45B5C4;
    struct Candidate
    {
        unsigned kind{ };
        std::vector<uint8_t> name;
        unsigned required_item{ }, ability_mask{ }, training_mask{ }, tier{ };
    };
    struct Request
    {
        unsigned unit_id{ }, source_kind{ }, target_kind{ };
        int training_group = -1;
        bool battlefield = true;
    };
    // The callback must revalidate the live unit and atomically apply the FULL
    // original transaction to live events AND the backing original save record.
    using Commit = std::function<bool(const Request&)>;
    enum class Page
    {
        selection,
        confirmation,
        closed
    };
    struct Result
    {
        bool changed = false;
        unsigned consumed_mask = 0;
        bool commit_blocked = false;
    };

private:
    std::vector<uint8_t> kinds_, bin_, pal_, cel_;
    OriginalMenu font_;
    size_t branches_{ }, records_{ }, growth_{ };
    unsigned count_{ };
    OriginalEvents::Unit unit_;
    std::vector<Candidate> choices_;
    Commit commit_;
    Page page_ = Page::closed;
    size_t selected_{ };
    int training_ = -1, return_code_ = 0;
    bool battlefield_ = true;
    unsigned word(size_t p) const { return kinds_.at(p) | (unsigned(kinds_.at(p + 1)) << 8); }
    unsigned dword(size_t p) const { return word(p) | (word(p + 2) << 16); }
    size_t record(unsigned kind) const
    {
        if (!kind || kind > count_)
        {
            throw std::invalid_argument("Invalid original promotion kind");
        }
        return records_ + size_t(kind - 1) * 37;
    }

public:
    OriginalPromotion() = default;
    static s2ne::Result<OriginalPromotion> create(const std::filesystem::path& root)
    {
        OriginalPromotion result;
        auto font = OriginalMenu::create(root);
        if (!font)
        {
            return s2ne::Result<OriginalPromotion>::failure(font.error());
        }
        result.font_ = std::move(font).value();
        for (auto pair : { std::pair{ &result.kinds_, "KINDSP.BIN" }, std::pair{ &result.bin_, "SUPERSP.BIN" }, std::pair{ &result.pal_, "SUPERSP.PAL" }, std::pair{ &result.cel_, "SUPERSP.CEL" } })
        {
            auto bytes = read_resource(root / pair.second);
            if (!bytes)
            {
                return s2ne::Result<OriginalPromotion>::failure(bytes.error());
            }
            *pair.first = std::move(bytes).value();
        }
        ResourceReader pal(result.pal_), kinds(result.kinds_), bin(result.bin_);
        unsigned growth = pal.number(904, 4);
        result.branches_ = kinds.number(0, 4);
        result.count_ = kinds.number(4);
        result.records_ = kinds.number(18, 4);
        if (!pal)
        {
            return s2ne::Result<OriginalPromotion>::failure(pal.error());
        }
        if (growth < 18 || result.pal_.size() < 768)
        {
            return s2ne::Result<OriginalPromotion>::failure(make_error("Invalid growth table address"));
        }
        result.growth_ = growth - 18;
        if (!result.count_ || !kinds.extent(result.branches_, size_t(result.count_) * 8) || !kinds.extent(result.records_, size_t(result.count_) * 37))
        {
            return s2ne::Result<OriginalPromotion>::failure(make_error("Invalid original promotion tables"));
        }
        if (!bin.extent(result.growth_ + 2, size_t(result.count_) * 158))
        {
            return s2ne::Result<OriginalPromotion>::failure(bin.error());
        }
        result.pal_[0] = result.pal_[1] = result.pal_[2] = 0;
        return s2ne::Result<OriginalPromotion>::success(std::move(result));
    }
    explicit OriginalPromotion(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    using BoundedRandom = std::function<unsigned(unsigned)>;
    // Exact 427A9B arithmetic with caller-supplied original 4732ED outcomes.
    // This does not pretend the Windows CRT rand() is the original runtime RNG.
    static s2ne::Result<int> try_growth_formula(const std::array<int, 6>& words, int level, const BoundedRandom& random)
    {
        if (!words[1] || !words[3])
        {
            return s2ne::Result<int>::success(0);
        }
        if (!random || words[4] < 0 || words[5] < 0 || level < 0)
        {
            return s2ne::Result<int>::failure(make_error("Invalid original promotion growth input"));
        }
        const unsigned divisor = unsigned(words[5] + 1), numerator = unsigned(words[4] + 1);
        unsigned denominator_draw = random(divisor);
        unsigned numerator_draw = random(numerator);
        if (denominator_draw >= divisor || numerator_draw >= numerator)
        {
            return s2ne::Result<int>::failure(make_error("Original bounded RNG result out of range"));
        }
        long double result = std::sqrt(static_cast<long double>(level) * words[0] / words[1]) + static_cast<long double>(words[2]) / words[3] + static_cast<long double>(numerator_draw) / (denominator_draw + 1) + 0.5L;
        if (!std::isfinite(result) || result < -32768 || result > 32767)
        {
            return s2ne::Result<int>::failure(make_error("Original growth result outside WORD range"));
        }
        return s2ne::Result<int>::success(int(result));
    }
    static int growth_formula(const std::array<int, 6>& words, int level, const BoundedRandom& random)
    {
        auto result = try_growth_formula(words, level, random);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    s2ne::Result<std::array<int, 9>> try_growth(unsigned kind, int level, const BoundedRandom& random) const
    {
        if (!kind || kind > count_)
        {
            return s2ne::Result<std::array<int, 9>>::failure(make_error("Invalid original promotion kind"));
        }
        size_t base = growth_ + 2 + size_t(kind - 1) * 108;
        if (base > bin_.size() || 108 > bin_.size() - base)
        {
            return s2ne::Result<std::array<int, 9>>::failure(make_error("Truncated original growth table"));
        }
        std::array<int, 9> values{ };
        for (unsigned index : { 0u, 1u, 2u, 0u, 3u, 4u, 5u, 6u, 7u, 8u })
        {
            std::array<int, 6> words{ };
            size_t p = base + index * 12;
            for (unsigned i = 0; i < 6; ++i)
            {
                words[i] = int(int16_t(unsigned(bin_[p + i * 2]) | (unsigned(bin_[p + i * 2 + 1]) << 8)));
            }
            auto value = try_growth_formula(words, level, random);
            if (!value)
            {
                return s2ne::Result<std::array<int, 9>>::failure(value.error());
            }
            values[index] = value.value();
        }
        return s2ne::Result<std::array<int, 9>>::success(values);
    }
    s2ne::Result<std::array<int, 9>> try_growth(unsigned kind, int level, OriginalRandom& random, const OriginalRandom::TimeSource& time = { }) const
    {
        auto next = random;
        std::optional<Error> error;
        auto result = try_growth(kind, level, [&](unsigned bound)
            {
                auto value = next.try_bounded(int32_t(bound), time);
                if (!value)
                {
                    if (!error)
                    {
                        error = value.error();
                    }
                    return 0u;
                }
                return value.value();
            });
        if (error)
        {
            return s2ne::Result<std::array<int, 9>>::failure(std::move(*error));
        }
        if (!result)
        {
            return result;
        }
        random = next;
        return result;
    }
    std::array<int, 9> growth(unsigned kind, int level, const BoundedRandom& random) const
    {
        auto result = try_growth(kind, level, random);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    std::array<int, 9> growth(unsigned kind, int level, OriginalRandom& random, const OriginalRandom::TimeSource& time = { }) const
    {
        auto result = try_growth(kind, level, random, time);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    std::array<int, 9> baseline(unsigned kind) const
    {
        record(kind);
        std::array<int, 9> result{ };
        size_t p = growth_ + 2 + size_t(count_) * 108 + size_t(kind - 1) * 50;
        for (size_t i = 0; i < 9; ++i)
        {
            result[i] = int(int16_t(unsigned(bin_.at(p + 2 * i)) | (unsigned(bin_.at(p + 2 * i + 1)) << 8)));
        }
        return result;
    }
    unsigned class_count() const { return count_; }
    Candidate candidate(unsigned kind) const
    {
        size_t p = record(kind);
        Candidate c;
        c.kind = kind;
        for (size_t i = 0; i < 14 && kinds_.at(p + i); ++i)
        {
            c.name.push_back(kinds_.at(p + i));
        }
        c.required_item = word(p + 27);
        c.ability_mask = dword(p + 29);
        c.training_mask = word(p + 35);
        c.tier = kinds_.at(p + 34);
        return c;
    }
    unsigned experience_threshold(unsigned kind) const { return word(record(kind) + 14); }
    // 4409B7 mutates EXP only for a terminal class, after level/mode gates.
    bool eligible(OriginalEvents::Unit& unit, unsigned runtime_threshold, int game_mode = 0) const
    {
        record(unit.appearance);
        if (unit.level < 10 || (unit.level == 10 && unit.experience < runtime_threshold) || game_mode == 1)
        {
            return false;
        }
        if (kinds_.at(branches_ + size_t(unit.appearance - 1) * 8))
        {
            return true;
        }
        if (unit.experience > runtime_threshold)
        {
            unit.experience = runtime_threshold;
        }
        return false;
    }
    bool target_allowed(const OriginalEvents::Unit& unit, unsigned kind) const
    {
        const auto c = candidate(kind);
        if (c.ability_mask && !(c.ability_mask & unit.abilities))
        {
            return false;
        }
        return !c.required_item || std::find(unit.equipment.begin(), unit.equipment.end(), c.required_item) != unit.equipment.end();
    }
    std::vector<Candidate> candidates(const OriginalEvents::Unit& unit) const
    {
        record(unit.appearance);
        std::vector<Candidate> result;
        size_t p = branches_ + size_t(unit.appearance - 1) * 8;
        for (unsigned i = 0; i < 8; ++i)
        {
            unsigned kind = kinds_.at(p + i);
            if (!kind)
            {
                break;
            }
            if (target_allowed(unit, kind))
            {
                result.push_back(candidate(kind));
            }
        }
        return result;
    }
    bool open(OriginalEvents::Unit unit, unsigned runtime_threshold, Commit commit = { }, bool battlefield = true, int game_mode = 0)
    {
        page_ = Page::closed;
        choices_.clear();
        selected_ = 0;
        training_ = -1;
        return_code_ = 0;
        commit_ = std::move(commit);
        battlefield_ = battlefield;
        if (!eligible(unit, runtime_threshold, game_mode))
        {
            return false;
        }
        unit_ = std::move(unit);
        choices_ = candidates(unit_);
        if (choices_.empty())
        {
            return false;    // Avoid original's zero-choice array dereference.
        }
        page_ = Page::selection;
        return true;
    }
    Page page() const { return page_; }
    const std::vector<Candidate>& choices() const { return choices_; }
    size_t selected_index() const { return selected_; }
    int training_group() const { return training_; }
    int return_code() const { return return_code_; }
    const OriginalEvents::Unit& unit() const { return unit_; }
    const Candidate& selected() const { return choices_.at(selected_); }
    std::optional<Request> request() const
    {
        if (page_ != Page::confirmation)
        {
            return std::nullopt;
        }
        return Request{ unit_.id, unit_.appearance, selected().kind, training_, battlefield_ };
    }
    void choose(size_t index)
    {
        if (page_ == Page::selection && index < choices_.size())
        {
            selected_ = index;
        }
    }
    void inspect()
    {
        if (page_ != Page::selection)
        {
            return;
        }
        page_ = Page::confirmation;
        training_ = -1;
        for (int i = 0; i < 11; ++i)
        {
            if (selected().training_mask & (1u << i))
            {
                training_ = i;
                break;
            }
        }
    }
    bool train(int group)
    {
        if (page_ != Page::confirmation || group < 0 || group >= 11 || !(selected().training_mask & (1u << group)))
        {
            return false;
        }
        training_ = group;
        return true;
    }
    void cancel()
    {
        if (page_ == Page::confirmation)
        {
            page_ = Page::selection;
            training_ = -1;
        }
        else if (page_ == Page::selection)
        {
            page_ = Page::closed;
            return_code_ = 0;
        }
    }
    bool confirm()
    {
        auto pending = request();
        if (!pending || !commit_ || !commit_(*pending))
        {
            return false;
        }
        page_ = Page::closed;
        return_code_ = 2;
        return true;
    }
    std::vector<uint8_t> draw(const std::vector<uint8_t>& background) const
    {
        if (background.size() != 640 * 480 * 4)
        {
            throw std::invalid_argument("Promotion requires640x480 RGBA");
        }
        auto image = background;
        if (page_ == Page::closed)
        {
            return image;
        }
        auto w = [](const auto& b, size_t p)
        {
            return unsigned(b.at(p)) | (unsigned(b.at(p + 1)) << 8);
        };
        auto dw = [&](const auto& b, size_t p)
        {
            return w(b, p) | (w(b, p + 2) << 16);
        };
        auto pixel = [&](int x, int y, unsigned ink)
        {
            if (x < 0 || x >= 640 || y < 0 || y >= 480)
            {
                return;
            }
            size_t p = (size_t(y) * 640 + x) * 4;
            for (unsigned c = 0; c < 3; ++c)
            {
                image[p + c] = uint8_t(unsigned(pal_.at(ink * 3 + c)) * 255 / 63);
            }
            image[p + 3] = 255;
        };
        auto sprite = [&](unsigned id, int left, int top)
        {
            size_t group = dw(pal_, 816) - 57;
            size_t p = group + dw(cel_, group + 2 + (id - 142) * 8);
            unsigned width = w(cel_, p) + 1, height = w(cel_, p + 2) + 1;
            p += 4;
            for (unsigned y = 0; y < height; ++y)
            {
                for (unsigned x = 0; x < width;)
                {
                    int op = int(int8_t(cel_.at(p++)));
                    unsigned n = op ? unsigned(op) & 127 : 1;
                    if (!n || n > width - x)
                    {
                        throw std::runtime_error("Invalid promotion CEL run");
                    }
                    unsigned repeat = op > 0 ? cel_.at(p++) : 0;
                    while (n--)
                    {
                        unsigned ink = op < 0 ? cel_.at(p++) : repeat;
                        if (op < 0 || ink)
                        {
                            pixel(left + int(x), top + int(y), ink);
                        }
                        ++x;
                    }
                }
            }
        };
        auto text = [&](const std::vector<uint8_t>& t, int x, int y, unsigned ink)
        {
            font_.draw_original_large(image, t, x, y, ink);
        };
        auto substitute = [](std::vector<uint8_t> format, const std::vector<std::vector<uint8_t>>& values)
        {
            std::vector<uint8_t> out;
            size_t value = 0;
            for (size_t i = 0; i < format.size(); ++i)
            {
                if (format[i] == '%' && i + 1 < format.size() && format[i + 1] == 's' && value < values.size())
                {
                    const auto& v = values[value++];
                    out.insert(out.end(), v.begin(), v.end());
                    ++i;
                }
                else
                {
                    out.push_back(format[i]);
                }
            }
            return out;
        };
        sprite(210, 0, 0);
        text(substitute(font_.text_bytes(178), { unit_.name }), 34, 14, 186);
        for (size_t i = 0; i < choices_.size(); ++i)
        {
            int x = 101 + int(i % 4) * 145, y = i < 4 ? 64 : 270;
            text(choices_[i].name, x - int(choices_[i].name.size()) * 6, y, i == selected_ ? 40 : 51);
            if (i == selected_)
            {
                for (int xx = x - 65; xx <= x + 65; ++xx)
                {
                    pixel(xx, y - 8, 40);
                    pixel(xx, y + 192, 40);
                }
                for (int yy = y - 8; yy <= y + 192; ++yy)
                {
                    pixel(x - 65, yy, 40);
                    pixel(x + 65, yy, 40);
                }
            }
        }
        if (page_ == Page::confirmation)
        {
            sprite(231, 11, 25);
            text(substitute(font_.text_bytes(179), { unit_.name, candidate(unit_.appearance).name, selected().name }), 21, 39, 54);
            text(font_.text_bytes(184), 35, 418, 40);
            for (int i = 0; i < 11; ++i)
            {
                auto label = font_.text_bytes(161 + i);
                font_.draw_small(image, label, 423, 118 + i * 26, selected().training_mask & (1u << i) ? 54 : 245, true);
                if (i == training_)
                {
                    for (int x = 375; x <= 615; ++x)
                    {
                        pixel(x, 117 + i * 26, 40);
                        pixel(x, 141 + i * 26, 40);
                    }
                }
            }
        }
        return image;
    }
    Result event(const OriginalSettings::Event& e)
    {
        if (page_ == Page::closed)
        {
            return { };
        }
        bool key = e.type == 1 || e.type == 2, mouse = e.type == 8 || e.type == 0x20;
        if ((key && e.scan == 1) || (mouse && (e.buttons & 2)))
        {
            cancel();
            return { true, key ? 4u : 16u };
        }
        if (page_ == Page::confirmation)
        {
            if (mouse && e.x >= 19 && e.x <= 197 && e.y >= 414 && e.y <= 448 && (e.buttons & 1))
            {
                bool accepted = confirm();
                return { accepted, 16, !accepted };
            }
            if (mouse && e.x >= 375 && e.x <= 615 && e.y >= 118 && e.y < 404)
            {
                int group = (e.y - 118) / 26;
                if ((selected().training_mask & (1u << group)) && (e.buttons & 1))
                {
                    return { train(group), 16 };
                }
            }
            // 459FCF keyboard activation/navigation handlers are no-ops.
            return { };
        }
        if (key)
        {
            switch (e.scan)
            {
            case 0x1C:
            case 0x39: inspect(); return { true, 4 };
            case 0x0F:
            case 0x4D: selected_ = (selected_ + 1) % choices_.size(); break;
            case 0x4B: selected_ = (selected_ + choices_.size() - 1) % choices_.size(); break;
            case 0x48:
            case 0x50:
                if (choices_.size() > 4)
                {
                    selected_ = std::min(selected_ < 4 ? selected_ + 4 : selected_ - 4, choices_.size() - 1);
                }
                break;
            default: return { };
            }
            return { true, 0 };
        }
        if (mouse)
        {
            for (size_t i = 0; i < choices_.size(); ++i)
            {
                int x = 36 + int(i % 4) * 146, y = i < 4 ? 55 : 261;
                if (e.x < x || e.x > x + 130 || e.y < y || e.y > y + 200)
                {
                    continue;
                }
                if (i != selected_)
                {
                    selected_ = i;
                    return { true, 0 };
                }
                if (e.buttons & 1)
                {
                    inspect();
                    return { true, 16 };
                }
                break;
            }
        }
        return { };
    }
};
}    //namespace s2ne
