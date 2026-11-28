#include "promotion.h"

namespace s2ne
{
auto Promotion::word(size_t p) const -> unsigned { return kinds_.at(p) | (unsigned(kinds_.at(p + 1)) << 8); }

auto Promotion::dword(size_t p) const -> unsigned { return word(p) | (word(p + 2) << 16); }

auto Promotion::try_record(unsigned kind) const -> s2ne::Result<size_t>
{
    if (!kind || kind > count_)
    {
        return s2ne::Result<size_t>::failure(make_error("Invalid original promotion kind"));
    }
    return s2ne::Result<size_t>::success(records_ + size_t(kind - 1) * 37);
}

auto Promotion::record(unsigned kind) const -> size_t
{
    auto result = try_record(kind);
    return std::move(result).value();
}

auto Promotion::confirmation_origin() const -> s2ne::Result<std::pair<int, int>>
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
    if (!data) { return s2ne::Result<std::pair<int, int>>::failure(make_error("Invalid promotion confirmation CEL dimensions")); }
    return s2ne::Result<std::pair<int, int>>::success({ 320 - width / 2, 240 - height / 2 });
}

auto Promotion::create(const std::filesystem::path& root) -> s2ne::Result<Promotion>
{
    Promotion result;
    auto font = Menu::create(root);
    if (!font)
    {
        return s2ne::Result<Promotion>::failure(font.error());
    }
    result.font_ = std::move(font).value();
    for (auto pair : { std::pair{ &result.kinds_, "KINDSP.BIN" }, std::pair{ &result.bin_, "SUPERSP.BIN" }, std::pair{ &result.pal_, "SUPERSP.PAL" }, std::pair{ &result.cel_, "SUPERSP.CEL" }, std::pair{ &result.prg_, "SUPERSP.PRG" } })
    {
        auto bytes = read_shared_resource(root / pair.second);
        if (!bytes)
        {
            return s2ne::Result<Promotion>::failure(bytes.error());
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
        return s2ne::Result<Promotion>::failure(pal.error());
    }
    if (growth < 18 || result.pal_.size() < 768)
    {
        return s2ne::Result<Promotion>::failure(make_error("Invalid growth table address"));
    }
    result.growth_ = growth - 18;
    if (!result.count_ || !kinds.extent(result.branches_, size_t(result.count_) * 8) || !kinds.extent(result.records_, size_t(result.count_) * 37))
    {
        return s2ne::Result<Promotion>::failure(make_error("Invalid original promotion tables"));
    }
    if (!bin.extent(result.growth_ + 2, size_t(result.count_) * 158))
    {
        return s2ne::Result<Promotion>::failure(bin.error());
    }
    result.pal_.mutable_at(0) = result.pal_.mutable_at(1) = result.pal_.mutable_at(2) = 0;
    return s2ne::Result<Promotion>::success(std::move(result));
}

Promotion::Promotion(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto Promotion::try_growth_formula(const std::array<int, 6>& words, int level, const BoundedRandom& random) -> s2ne::Result<int>
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

auto Promotion::growth_formula(const std::array<int, 6>& words, int level, const BoundedRandom& random) -> int
{
    auto result = try_growth_formula(words, level, random);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto Promotion::try_growth(unsigned kind, int level, const BoundedRandom& random, bool maximize) const -> s2ne::Result<std::array<int, 9>>
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
    if (!random) { return s2ne::Result<std::array<int, 9>>::failure(make_error("Missing original growth RNG")); }
    std::array<int, 9> values{};
    for (unsigned index : { 0u, 1u, 2u, 0u, 3u, 4u, 5u, 6u, 7u, 8u })
    {
        std::array<int, 6> words{};
        size_t p = base + index * 12;
        for (unsigned i = 0; i < 6; ++i)
        {
            words[i] = int(int16_t(unsigned(bin_[p + i * 2]) | (unsigned(bin_[p + i * 2 + 1]) << 8)));
        }
        unsigned draw = 0;
        auto value = try_growth_formula(words, level, [&](unsigned bound)
            {
                const auto result = random(bound);
                if (result >= bound || !maximize) { return result; }
                return draw++ % 2 == 0 ? 0u : bound - 1;
            });
        if (!value)
        {
            return s2ne::Result<std::array<int, 9>>::failure(value.error());
        }
        values[index] = value.value();
    }
    return s2ne::Result<std::array<int, 9>>::success(values);
}

auto Promotion::try_growth(unsigned kind, int level, RandomStream& random, const RandomStream::TimeSource& time, bool maximize) const -> s2ne::Result<std::array<int, 9>>
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
        },
        maximize);
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

auto Promotion::growth(unsigned kind, int level, const BoundedRandom& random) const -> std::array<int, 9>
{
    auto result = try_growth(kind, level, random);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto Promotion::growth(unsigned kind, int level, RandomStream& random, const RandomStream::TimeSource& time) const -> std::array<int, 9>
{
    auto result = try_growth(kind, level, random, time);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto Promotion::baseline(unsigned kind) const -> std::array<int, 9>
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

auto Promotion::class_count() const -> unsigned { return count_; }

auto Promotion::set_preview_source(PreviewSource source) -> void { preview_source_ = std::move(source); }

auto Promotion::try_preview(unsigned target, RandomStream& random, const RandomStream::TimeSource& time) const -> s2ne::Result<Preview>
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

auto Promotion::candidate(unsigned kind) const -> Candidate
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

auto Promotion::experience_threshold(unsigned kind) const -> unsigned { return word(record(kind) + 14); }

auto Promotion::innate_ability(unsigned kind) const -> unsigned
{
    unsigned index = kinds_.at(record(kind) + 19);
    if (!index) { return 0; }
    return 1u << ((index - 1) & 31);
}

auto Promotion::eligible(Events::Unit& unit, unsigned runtime_threshold, int game_mode) const -> bool
{
    record(unit.appearance);
    const bool assist = Settings::gameplay_assist && !unit.hostile();
    if (unit.level < 10 || (!assist && unit.level == 10 && unit.experience < runtime_threshold) || game_mode == 1)
    {
        return false;
    }
    if (kinds_.at(branches_ + size_t(unit.appearance - 1) * 8))
    {
        return true;
    }
    if (!assist && unit.experience > runtime_threshold)
    {
        unit.experience = runtime_threshold;
    }
    return false;
}

auto Promotion::target_allowed(const Events::Unit& unit, unsigned kind) const -> bool
{
    const auto c = candidate(kind);
    if (c.ability_mask && !(c.ability_mask & unit.abilities))
    {
        return false;
    }
    return !c.required_item || std::find(unit.equipment.begin(), unit.equipment.end(), c.required_item) != unit.equipment.end();
}

auto Promotion::candidates(const Events::Unit& unit) const -> std::vector<Candidate>
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
    if (protagonist_class_tree && (unit.id == 11 || unit.id == 1))
    {
        const unsigned base = unit.id == 11 ? 81u : 87u;
        const unsigned first = unit.id == 11 ? 1u : 36u;
        const bool exclusive_highest = unit.appearance >= base + 3 && unit.appearance <= base + 5;
        const bool ordinary_highest = candidate(unit.appearance).tier == 5 && unit.appearance >= first && unit.appearance < first + 35;
        if (exclusive_highest || ordinary_highest)
        {
            for (unsigned kind = first; kind < first + 9; ++kind)
            {
                if (target_allowed(unit, kind) && std::none_of(result.begin(), result.end(), [kind](const auto& c)
                        {
                            return c.kind == kind;
                        }))
                {
                    result.push_back(candidate(kind));
                }
            }
            if (target_allowed(unit, base) && std::none_of(result.begin(), result.end(), [base](const auto& c)
                    {
                        return c.kind == base;
                    }))
            {
                result.push_back(candidate(base));
            }
        }
    }
    return result;
}

auto Promotion::open(Events::Unit unit, unsigned runtime_threshold, Commit commit, bool battlefield, int game_mode) -> bool
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

auto Promotion::page() const -> Page { return page_; }

auto Promotion::choices() const -> const std::vector<Candidate>& { return choices_; }

auto Promotion::selected_index() const -> size_t { return selected_; }

auto Promotion::training_group() const -> int { return training_; }

auto Promotion::return_code() const -> int { return return_code_; }

auto Promotion::unit() const -> const Events::Unit& { return unit_; }

auto Promotion::selected() const -> const Candidate& { return choices_.at(selected_); }

auto Promotion::tick(uint64_t milliseconds) -> bool
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

auto Promotion::request() const -> std::optional<Request>
{
    if (page_ != Page::confirmation)
    {
        return std::nullopt;
    }
    return Request{ unit_.id, unit_.appearance, selected().kind, training_, battlefield_ };
}

auto Promotion::choose(size_t index) -> void
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

auto Promotion::inspect() -> void
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

auto Promotion::train(int group) -> bool
{
    if (page_ != Page::confirmation || group < 0 || group >= 11 || !(selected().training_mask & (1u << group)))
    {
        return false;
    }
    training_ = group;
    return true;
}

auto Promotion::cancel() -> void
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

auto Promotion::confirm() -> bool
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

auto Promotion::try_draw(const std::vector<uint8_t>& background) const -> s2ne::Result<std::vector<uint8_t>>
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
        auto rendered = draw_original_cel(image, cel_, pal_, id, left, top, ZeroRun::transparent);
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
    if (page_ == Page::selection)
    {
        sprite(210, 0, 0);
        const bool compact = choices_.size() > 8;
        if (compact)
        {
            const auto original_panel = image;
            for (int y = 55; y < 469; ++y)
            {
                const int local = (y - 55) % 138;
                int source_y = 55 + local;
                if (local >= 38) { source_y = 93 + (local - 38) * 168 / 100; }
                std::copy_n(original_panel.begin() + (size_t(source_y) * 640 + 8) * 4, 620 * 4,
                    image.begin() + (size_t(y) * 640 + 8) * 4);
            }
        }
        text(substitute(font_.text_bytes(178), { unit_.name }), 34, 14, 186);
        const int row_height = compact ? 138 : 206;
        const int cell_height = compact ? 132 : 200;
        for (size_t i = 0; i < choices_.size(); ++i)
        {
            int x = 101 + int(i % 4) * 145, y = 64 + int(i / 4) * row_height;
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
            if (compact && (w > 640 || h > 480))
            {
                return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Promotion class frame exceeds compact decode surface"));
            }
            std::vector<uint8_t> scaled_source;
            if (compact) { scaled_source.resize(image.size()); }
            int sprite_x = 0, sprite_y = 0;
            if (!compact)
            {
                sprite_x = x + (i == selected_ ? int(i % 4) : 0) - int(w / 2);
                sprite_y = y + 190 - int(h);
            }
            auto rendered = draw_original_sprite(compact ? scaled_source : image, std::span<const uint8_t>(prg_).subspan(frames + offset, length), pal_, 0, false,
                sprite_x, sprite_y, ZeroRun::transparent);
            if (!rendered) { return s2ne::Result<std::vector<uint8_t>>::failure(rendered.error()); }
            if (compact)
            {
                const unsigned scale = std::max({ 110u, h, (w * 110 + 119) / 120 });
                const unsigned dw = std::max(1u, w * 110 / scale), dh = std::max(1u, h * 110 / scale);
                for (unsigned sy = 0; sy < dh; ++sy)
                {
                    for (unsigned sx = 0; sx < dw; ++sx)
                    {
                        const size_t source = (size_t(sy * h / dh) * 640 + sx * w / dw) * 4;
                        if (!scaled_source[source + 3]) { continue; }
                        const size_t target = (size_t(y + 122 - int(dh) + int(sy)) * 640 + x - int(dw / 2) + sx) * 4;
                        std::copy_n(scaled_source.begin() + source, 4, image.begin() + target);
                    }
                }
            }
            if (i == selected_)
            {
                for (int xx = x - 65; xx <= x + 65; ++xx)
                {
                    pixel(xx, y - 8, 40);
                    pixel(xx, y + cell_height - 8, 40);
                }
                for (int yy = y - 8; yy <= y + cell_height - 8; ++yy)
                {
                    pixel(x - 65, yy, 40);
                    pixel(x + 65, yy, 40);
                }
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
    if (sprite_error)
    {
        return s2ne::Result<std::vector<uint8_t>>::failure(*sprite_error);
    }
    return s2ne::Result<std::vector<uint8_t>>::success(image);
}

auto Promotion::draw(const std::vector<uint8_t>& background) const -> std::vector<uint8_t>
{
    auto result = try_draw(background);
    return std::move(result).value();
}

auto Promotion::event(const Settings::Event& e) -> Result
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
                const size_t rows = (choices_.size() + 3) / 4;
                const size_t row = selected_ / 4;
                const size_t next = e.scan == 0x48 ? (row + rows - 1) % rows : (row + 1) % rows;
                selected_ = std::min(next * 4 + selected_ % 4, choices_.size() - 1);
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
            const bool compact = choices_.size() > 8;
            int x = 36 + int(i % 4) * 146, y = 55 + int(i / 4) * (compact ? 138 : 206);
            if (e.x < x || e.x > x + 130 || e.y < y || e.y > y + (compact ? 132 : 200))
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
}    //namespace s2ne
