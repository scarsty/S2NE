#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_random.hpp"
#include "original_settings.hpp"
#include "original_sprite.hpp"
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
        unsigned kind{};
        std::vector<uint8_t> name;
        unsigned required_item{}, ability_mask{}, training_mask{}, tier{};
    };
    struct Request
    {
        unsigned unit_id{}, source_kind{}, target_kind{};
        int training_group = -1;
        bool battlefield = true;
    };
    // The callback must revalidate the live unit and atomically apply the FULL
    // original transaction to live events AND the backing original save record.
    using Commit = std::function<bool(const Request&)>;
    struct Preview
    {
        std::array<int, 10> current{}, promoted{}, reference{};
    };
    using PreviewSource = std::function<s2ne::Result<Preview>(unsigned)>;
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
    SharedBytes kinds_, bin_, pal_, cel_, prg_;
    OriginalMenu font_;
    size_t branches_{}, records_{}, growth_{};
    unsigned count_{};
    OriginalEvents::Unit unit_;
    std::vector<Candidate> choices_;
    Commit commit_;
    PreviewSource preview_source_;
    std::optional<Preview> preview_;
    Page page_ = Page::closed;
    size_t selected_{};
    std::optional<uint64_t> animation_tick_;
    unsigned animation_frame_{};
    int training_ = -1, confirmation_hover_ = 11, return_code_ = 0;
    bool battlefield_ = true;
    unsigned word(size_t p) const { return kinds_.at(p) | (unsigned(kinds_.at(p + 1)) << 8); }
    unsigned dword(size_t p) const { return word(p) | (word(p + 2) << 16); }
    s2ne::Result<size_t> try_record(unsigned kind) const
    {
        if (!kind || kind > count_)
        {
            return s2ne::Result<size_t>::failure(make_error("Invalid original promotion kind"));
        }
        return s2ne::Result<size_t>::success(records_ + size_t(kind - 1) * 37);
    }
    size_t record(unsigned kind) const
    {
        auto result = try_record(kind);
        return std::move(result).value();
    }
    s2ne::Result<std::pair<int, int>> confirmation_origin() const
    {
        ResourceReader palette(pal_), data(cel_);
        const unsigned address = palette.number(816, 4);
        if (!palette || address < 57) { return s2ne::Result<std::pair<int, int>>::failure(make_error("Invalid promotion CEL directory")); }
        const size_t group = address - 57;
        const unsigned count = data.number(group);
        if (count <= 231 - 142) { return s2ne::Result<std::pair<int, int>>::failure(make_error("Missing promotion confirmation CEL")); }
        const size_t entry = group + 2 + size_t(231 - 142) * 8;
        const size_t offset = data.number(entry, 4), length = data.number(entry + 4, 4);
        if (!data.extent(group + offset, length) || length < 4) { return s2ne::Result<std::pair<int, int>>::failure(make_error("Invalid promotion confirmation CEL extent")); }
        const int width = int(data.number(group + offset)) + 1, height = int(data.number(group + offset + 2)) + 1;
        if (!data || width > 640 || height > 480) { return s2ne::Result<std::pair<int, int>>::failure(make_error("Invalid promotion confirmation CEL dimensions")); }
        return s2ne::Result<std::pair<int, int>>::success({ 320 - width / 2, 240 - height / 2 });
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
        for (auto pair : { std::pair{ &result.kinds_, "KINDSP.BIN" }, std::pair{ &result.bin_, "SUPERSP.BIN" }, std::pair{ &result.pal_, "SUPERSP.PAL" }, std::pair{ &result.cel_, "SUPERSP.CEL" }, std::pair{ &result.prg_, "SUPERSP.PRG" } })
        {
            auto bytes = read_shared_resource(root / pair.second);
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
        result.pal_.mutable_at(0) = result.pal_.mutable_at(1) = result.pal_.mutable_at(2) = 0;
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
        std::array<int, 9> values{};
        for (unsigned index : { 0u, 1u, 2u, 0u, 3u, 4u, 5u, 6u, 7u, 8u })
        {
            std::array<int, 6> words{};
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
    s2ne::Result<std::array<int, 9>> try_growth(unsigned kind, int level, OriginalRandom& random, const OriginalRandom::TimeSource& time = {}) const
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
    std::array<int, 9> growth(unsigned kind, int level, OriginalRandom& random, const OriginalRandom::TimeSource& time = {}) const
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
        std::array<int, 9> result{};
        size_t p = growth_ + 2 + size_t(count_) * 108 + size_t(kind - 1) * 50;
        for (size_t i = 0; i < 9; ++i)
        {
            result[i] = int(int16_t(unsigned(bin_.at(p + 2 * i)) | (unsigned(bin_.at(p + 2 * i + 1)) << 8)));
        }
        return result;
    }
    unsigned class_count() const { return count_; }
    void set_preview_source(PreviewSource source) { preview_source_ = std::move(source); }
    s2ne::Result<Preview> try_preview(unsigned target, OriginalRandom& random, const OriginalRandom::TimeSource& time) const
    {
        auto staged = random;
        std::array<int, 9> reference{};
        for (int level = 1; level <= 10; ++level)
        {
            auto result = try_growth(target, level, staged, time);
            if (!result) { return s2ne::Result<Preview>::failure(result.error()); }
            for (size_t i = 0; i < reference.size(); ++i) { reference[i] = int(int16_t(reference[i] + result.value()[i])); }
        }
        auto growth = try_growth(target, 1, staged, time);
        if (!growth) { return s2ne::Result<Preview>::failure(growth.error()); }
        const auto base = baseline(target);
        const std::array<int, 9> values{ unit_.max_hp, unit_.max_mp, unit_.max_st, unit_.attack_limit, unit_.defense_limit, unit_.strikeback, unit_.base_tactics, unit_.max_sp, unit_.fencing };
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (values[i] < base[i]) { growth.value()[i] = int(int16_t(growth.value()[i] + (base[i] - values[i]) / 2)); }
        }
        Preview result;
        result.current = { unit_.max_hp, unit_.max_mp, unit_.max_st, unit_.max_sp, unit_.strikeback, unit_.attack_limit, unit_.base_action_points, unit_.max_st, unit_.fencing, unit_.defense_limit };
        constexpr std::array<int, 10> indices{ 0, 1, 2, -1, 3, 4, 5, 7, 6, 8 };
        for (size_t row = 0; row < indices.size(); ++row)
        {
            const int index = indices[row];
            if (index >= 0)
            {
                result.promoted[row] = result.current[row] + growth.value()[size_t(index)];
                result.reference[row] = reference[size_t(index)];
            }
        }
        int delta = std::max(int(int8_t(kinds_.at(record(target) + 21))) - (unit_.max_sp - unit_.equipment_stat_deltas[2]), 0);
        if (int8_t(kinds_.at(record(target) + 17)) != 5 && int8_t(kinds_.at(record(unit_.appearance) + 17)) == 5) { delta = -4; }
        result.promoted[3] = unit_.max_sp + delta;
        random = staged;
        return s2ne::Result<Preview>::success(result);
    }
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
    unsigned innate_ability(unsigned kind) const
    {
        unsigned index = kinds_.at(record(kind) + 19);
        if (!index) { return 0; }
        return 1u << ((index - 1) & 31);
    }
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
    bool open(OriginalEvents::Unit unit, unsigned runtime_threshold, Commit commit = {}, bool battlefield = true, int game_mode = 0)
    {
        page_ = Page::closed;
        choices_.clear();
        selected_ = 0;
        animation_tick_.reset();
        animation_frame_ = 0;
        training_ = -1;
        preview_.reset();
        preview_source_ = {};
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
    bool tick(uint64_t milliseconds)
    {
        if (page_ != Page::selection) { return false; }
        if (!animation_tick_)
        {
            animation_tick_ = milliseconds;
            return false;
        }
        if (milliseconds - *animation_tick_ < 190) { return false; }
        animation_tick_ = milliseconds;
        animation_frame_ ^= 1;
        return true;
    }
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
            if (index != selected_)
            {
                animation_tick_.reset();
                animation_frame_ = 0;
            }
            selected_ = index;
        }
    }
    void inspect()
    {
        if (page_ != Page::selection)
        {
            return;
        }
        preview_.reset();
        if (preview_source_)
        {
            auto preview = preview_source_(selected().kind);
            if (!preview)
            {
                report_error(preview.error());
                return;
            }
            preview_ = preview.value();
        }
        page_ = Page::confirmation;
        training_ = -1;
        confirmation_hover_ = 11;
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
            animation_tick_.reset();
            animation_frame_ = 0;
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
    s2ne::Result<std::vector<uint8_t>> try_draw(const std::vector<uint8_t>& background) const
    {
        if (background.size() != 640 * 480 * 4)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Promotion requires640x480 RGBA"));
        }
        auto image = background;
        if (page_ == Page::closed)
        {
            return s2ne::Result<std::vector<uint8_t>>::success(image);
        }
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
        std::optional<Error> sprite_error;
        auto sprite = [&](unsigned id, int left, int top)
        {
            auto rendered = draw_original_cel(image, cel_, pal_, id, left, top, OriginalZeroRun::transparent);
            if (!rendered)
            {
                sprite_error = rendered.error();
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
            ResourceReader palette(pal_), data(prg_);
            const unsigned index_address = palette.number(1320, 4), frame_address = palette.number(1328, 4);
            if (!palette || index_address < 10 || frame_address < 11)
            {
                return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Invalid promotion PRG directory"));
            }
            const size_t index = index_address - 10, frames = frame_address - 11;
            const unsigned first = data.number(index + 2 + size_t(choices_[i].kind - 1) * 8 + 2);
            const unsigned frame = first + (i == selected_ ? 4 + animation_frame_ : 0);
            const size_t entry = frames + 2 + size_t(frame) * 7;
            const size_t offset = data.number(entry, 4), length = data.number(entry + 4);
            if (!data.extent(frames + offset, length) || length < 4)
            {
                return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Invalid promotion class frame"));
            }
            const unsigned w = data.number(frames + offset) + 1, h = data.number(frames + offset + 2) + 1;
            auto rendered = draw_original_sprite(image, std::span<const uint8_t>(prg_).subspan(frames + offset, length), pal_, 0, false,
                x + (i == selected_ ? int(i % 4) : 0) - int(w / 2), y + 190 - int(h), OriginalZeroRun::transparent);
            if (!rendered) { return s2ne::Result<std::vector<uint8_t>>::failure(rendered.error()); }
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
            auto origin = confirmation_origin();
            if (!origin) { return s2ne::Result<std::vector<uint8_t>>::failure(origin.error()); }
            const auto [left, top] = origin.value();
            sprite(231, left, top);
            text(substitute(font_.text_bytes(179), { unit_.name, candidate(unit_.appearance).name, selected().name }), left + 10, top + 13, 116);
            const auto confirm_label = font_.text_bytes(184);
            text(confirm_label, left + 98 - int(confirm_label.size()) * 6, top + 393, 69);
            constexpr std::array<int, 4> heading_centers{ 124, 301, 431, 555 };
            for (size_t i = 0; i < heading_centers.size(); ++i)
            {
                const auto heading = font_.text_bytes(180 + unsigned(i));
                font_.draw_small(image, heading, left + heading_centers[i] - int(heading.size()) * 4, top + 54, 132, true);
            }
            if (preview_)
            {
                constexpr std::array<unsigned, 7> labels{ 13, 7, 8, 10, 11, 9, 12 };
                for (size_t row = 0; row < 10; ++row)
                {
                    std::vector<uint8_t> label;
                    if (row < 3)
                    {
                        constexpr std::array<const char*, 3> names{ "HP", "MP", "ST" };
                        const std::string name = names[row];
                        label.assign(name.begin(), name.end());
                    }
                    else
                    {
                        label = font_.text_bytes(labels[row - 3]);
                    }
                    const unsigned ink = row % 2 == 0 ? 162 : 185;
                    const int y = top + 92 + int(row) * 28;
                    text(label, left + 10, y, ink);
                    sprite(148, left + 136, y + 10);
                    const std::array<int, 3> numbers{ preview_->current[row], preview_->promoted[row], preview_->reference[row] };
                    constexpr std::array<int, 3> columns{ 74, 166, 270 };
                    for (size_t column = 0; column < columns.size(); ++column)
                    {
                        auto number = std::to_string(numbers[column]);
                        if (number.size() < 3) { number.insert(0, 3 - number.size(), ' '); }
                        text(std::vector<uint8_t>(number.begin(), number.end()), left + columns[column], y, ink);
                    }
                }
            }
            for (int i = 0; i < 11; ++i)
            {
                const bool allowed = (selected().training_mask & (1u << i)) != 0;
                const bool trained = i == training_;
                const int y = top + 93 + i * 26;
                sprite((trained ? 241 : 242) + unsigned(i) * 2, left + 366, y);
                const auto label = font_.text_bytes(161 + i);
                unsigned label_ink = allowed ? 54 : 245;
                unsigned number_ink = allowed ? 184 : 177;
                if (trained)
                {
                    label_ink = 69;
                    number_ink = 69;
                }
                text(label, left + 412, y, label_ink);
                const auto proficiency = std::to_string(unit_.learned_proficiency[size_t(i)] / 10);
                text(std::vector<uint8_t>(proficiency.begin(), proficiency.end()), left + 544, y, number_ink);
                if (i == confirmation_hover_)
                {
                    for (int x = left + 364; x <= left + 604; ++x)
                    {
                        pixel(x, top + 92 + i * 26, 40);
                        pixel(x, top + 116 + i * 26, 40);
                    }
                    for (int y = top + 92 + i * 26; y <= top + 116 + i * 26; ++y)
                    {
                        pixel(left + 364, y, 40);
                        pixel(left + 604, y, 40);
                    }
                }
                const unsigned ability = kinds_.at(record(selected().kind) + 19);
                if (ability)
                {
                    ResourceReader kinds(kinds_);
                    size_t offset = kinds.number(6, 4);
                    const unsigned count = kinds.number(10);
                    if (!kinds || ability >= count) { return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Invalid promotion innate ability string")); }
                    for (unsigned index = 0; index < ability; ++index)
                    {
                        while (offset < kinds_.size() && kinds_[offset]) { ++offset; }
                        ++offset;
                    }
                    auto label = font_.text_bytes(158);
                    label.push_back(' ');
                    while (offset < kinds_.size() && kinds_[offset]) { label.push_back(kinds_[offset++]); }
                    if (offset >= kinds_.size()) { return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Truncated promotion innate ability string")); }
                    text(label, left + 370, top + 394, 54);
                }
                if (confirmation_hover_ == 11)
                {
                    for (int x = left + 8; x <= left + 184; ++x)
                    {
                        pixel(x, top + 389, 40);
                        pixel(x, top + 421, 40);
                    }
                    for (int y = top + 389; y <= top + 421; ++y)
                    {
                        pixel(left + 8, y, 40);
                        pixel(left + 184, y, 40);
                    }
                }
            }
        }
        if (sprite_error)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(*sprite_error);
        }
        return s2ne::Result<std::vector<uint8_t>>::success(image);
    }
    std::vector<uint8_t> draw(const std::vector<uint8_t>& background) const
    {
        auto result = try_draw(background);
        return std::move(result).value();
    }
    Result event(const OriginalSettings::Event& e)
    {
        if (page_ == Page::closed)
        {
            return {};
        }
        bool key = e.type == 1 || e.type == 2, mouse = e.type == 8 || e.type == 0x20;
        if ((key && e.scan == 1) || (mouse && (e.buttons & 2)))
        {
            cancel();
            return { true, key ? 4u : 16u };
        }
        if (page_ == Page::confirmation)
        {
            auto origin = confirmation_origin();
            if (!origin)
            {
                report_error(origin.error());
                return {};
            }
            const auto [left, top] = origin.value();
            if (mouse && e.x >= left + 8 && e.x <= left + 186 && e.y >= top + 389 && e.y <= top + 423)
            {
                const bool changed = confirmation_hover_ != 11;
                confirmation_hover_ = 11;
                if (e.buttons & 1)
                {
                    bool accepted = confirm();
                    return { accepted, 16, !accepted };
                }
                return { changed, 0 };
            }
            if (mouse && e.x >= left + 366 && e.x <= left + 610 && e.y >= top + 93 && e.y < top + 379)
            {
                int group = (e.y - top - 93) / 26;
                if (selected().training_mask & (1u << group))
                {
                    const bool changed = confirmation_hover_ != group;
                    confirmation_hover_ = group;
                    if (e.buttons & 1) { return { train(group), 16 }; }
                    return { changed, 0 };
                }
            }
            // 459FCF keyboard activation/navigation handlers are no-ops.
            return {};
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
            default: return {};
            }
            animation_tick_.reset();
            animation_frame_ = 0;
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
                    choose(i);
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
        return {};
    }
};
}    //namespace s2ne
