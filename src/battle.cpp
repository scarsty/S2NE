#include "battle.h"

namespace s2ne
{
auto Battle::set_ai_skills(const OriginalSkills& rules) -> void
{
    if (!ai_skills_) { ai_skills_ = std::make_shared<OriginalSkills>(rules); }
}

auto Battle::set_ai_damage(const OriginalSkill94Transaction& rules) -> void
{
    if (!ai_damage_) { ai_damage_ = std::make_shared<OriginalSkill94Transaction>(rules); }
}

auto Battle::set_ai_skill_camera(std::pair<int, int> camera) -> void { ai_skill_camera_ = camera; }

auto Battle::ai_skill_pending() const -> bool { return ai_skill_.has_value() || ai_skill_waiting_ || ai_skill_failed_; }

auto Battle::ai_skill_request() const -> const std::optional<AiSkill>& { return ai_skill_; }

auto Battle::ai_skill_actor() const -> std::optional<size_t> { return ai_skill_actor_; }

auto Battle::ai_skill_ready() const -> bool { return !ai_skill_failed_ && ai_skill_ && !animating_movement() && hostile_path_cursor_ >= hostile_path_.size(); }

auto Battle::begin_ai_skill() -> void { ai_skill_waiting_ = true; }

auto Battle::fail_ai_skill() -> void
{
    ai_skill_failed_ = false;
    ai_skill_waiting_ = false;
    ai_skill_.reset();
    ai_skill_actor_.reset();
    hostile_path_.clear();
    hostile_path_cursor_ = 0;
    hostile_target_ = -1;
    hostile_attack_slot_ = -1;
    ai_skill_replan_ = true;
}

auto Battle::ai_skill_failed() const -> bool { return ai_skill_failed_; }

auto Battle::finish_ai_skill() -> void
{
    ai_skill_.reset();
    ai_skill_actor_.reset();
    ai_skill_waiting_ = false;
    hostile_path_.clear();
    hostile_path_cursor_ = 0;
    hostile_target_ = -1;
    hostile_attack_slot_ = -1;
    ai_skill_replan_ = true;
}

auto Battle::movement_sound(const OriginalEvents::Unit& pose) -> void
{
    if (!effect_audio_) { return; }
    unsigned cls = 0;
    if (hostile_resources_) { cls = hostile_resources_->movement_class(pose); }
    const auto& tile = cell({ pose.x, pose.y });
    unsigned terrain = 0;
    if (tile.ground > 0 && size_t(tile.ground) <= ground_attributes_.size()) { terrain = ground_attributes_[size_t(tile.ground - 1)] >> 8; }
    const unsigned id = original_movement_sound(cls, pose.movement_type, tile.ground, terrain);
    if (id != movement_sound_id_) { movement_sound_channel_ = 0; }
    movement_sound_id_ = id;
    movement_sound_channel_ = effect_audio_(id, movement_sound_channel_);
}

auto Battle::set_effect_audio(std::function<unsigned(unsigned, unsigned)> callback) -> void { effect_audio_ = std::move(callback); }

auto Battle::scripted_movement_sound_id(const OriginalEvents::Unit& pose) const -> unsigned
{
    if (!hostile_resources_) { return 0; }
    const auto& tile = cell({ pose.x, pose.y });
    unsigned terrain = 0;
    if (tile.ground > 0 && size_t(tile.ground) <= ground_attributes_.size()) { terrain = ground_attributes_[size_t(tile.ground - 1)] >> 8; }
    // 原45E450／461808只认42CD63返回1；不能沿用普通移动同时接受2的分支。
    if (hostile_resources_->movement_class(pose) == 1) { return 122; }
    if (tile.ground > 0 && tile.ground <= 4 && terrain == 1) { return 131; }
    if (pose.movement_type == 4) { return 124; }
    return 103;
}

auto Battle::scripted_effect_sound(unsigned id, unsigned channel) const -> unsigned { return effect_audio_ && id ? effect_audio_(id, channel) : 0; }

auto Battle::projectile_active() const -> bool { return projectile_.has_value(); }

auto Battle::physical_diagnostic() const -> const std::optional<PhysicalDiagnostic>& { return last_physical_diagnostic_; }

auto Battle::requested_command() const -> int { return requested_command_; }

auto Battle::requested_detail_unit() const -> int { return requested_detail_unit_; }

auto Battle::clear_requested_command() -> void
{
    requested_command_ = -1;
    requested_detail_unit_ = -1;
}

auto Battle::commit_external_action() -> void { acted_ = true; }

auto Battle::commit_equipment_action(OriginalEvents::Unit& actor) -> void
{
    actor.action_points = int(int16_t(actor.action_points - ai_config_[0xF2]));
    actor.carry_points = std::min(actor.carry_points, actor.action_points);
    actor.status34 |= 0x20;
}

auto Battle::set_command_request_availability(std::function<bool(const OriginalEvents::Unit&, int)> available) -> void { command_request_available_ = std::move(available); }

auto Battle::random_seconds() const -> uint32_t { return random_seconds_.value_or(uint32_t(std::time(nullptr))); }

auto Battle::percent() -> int { return random_.percent(random_seconds()); }

auto Battle::roll(int n) -> int { return random_.bounded(n, random_seconds()); }

auto Battle::recover_rest(std::vector<OriginalEvents::Unit>& units, size_t index) -> void
{
    auto& unit = units.at(index);
    if (physical_status(unit, 0x3E) & 1) { return; }
    int percentage = roll(11) + 10;
    if (OriginalSettings::gameplay_assist && !unit.hostile()) { percentage = 20; }
    auto restore = [percentage](int& current, int maximum, int limit, bool doubled)
    {
        if (maximum <= 0 || current >= maximum) { return 0; }
        int amount = maximum * percentage / 100;
        if (doubled) { amount *= 2; }
        amount = std::min({ std::max(5, amount), maximum - current, limit });
        current = int(int16_t(current + amount));
        return amount;
    };
    // 原45424D：同一次随机比例，MP仅装备特殊码63翻倍。
    std::array amounts{
        restore(unit.hp, unit.max_hp, 300, false),
        restore(unit.mp, unit.max_mp, 100, hostile_resources_ && hostile_resources_->equipment_special(unit, 63)),
        restore(unit.st, unit.max_st, 100, false)
    };
    if (std::none_of(amounts.begin(), amounts.end(), [](int amount)
            {
                return amount != 0;
            })
        || (unit.ai_role10 >= 0 ? unit.ai_role10 : unit.behavior) == 1)
    {
        return;
    }
    if (unit.hostile() && hostile_resources_ && hostile_resources_->army_modes && (*hostile_resources_->army_modes)[1] == 1)
    {
        std::vector<OriginalEvents::Unit> observers;
        for (const auto& observer : units)
        {
            if (!observer.hostile()) { observers.push_back(observer); }
        }
        if (!target_visible(unit, observers, 0)) { return; }
    }
    rest_feedback_ = RestFeedback{ index, amounts };
}

Battle::Battle(unsigned w, unsigned h, std::vector<Cell> cells) : width_(w), height_(h), cells_(std::move(cells))
{
    if (!w || !h || cells_.size() != size_t(w) * h)
    {
        fatal("Invalid battle terrain");
    }
    random_.initialize_with_clock([]
        {
            return uint32_t(std::time(nullptr));
        });
}

auto Battle::try_load(const std::filesystem::path& root, unsigned map, OriginalDisc disc) -> Result<Battle>
{
    auto prgResult = read_shared_resource(root / "SUPERSP.PRG"), palResult = read_shared_resource(root / "SUPERSP.PAL");
    if (!prgResult)
    {
        return Result<Battle>::failure(prgResult.error());
    }
    if (!palResult)
    {
        return Result<Battle>::failure(palResult.error());
    }
    auto prg = std::move(prgResult).value(), pal = std::move(palResult).value();
    ResourceReader p(pal), r(prg);
    unsigned raw = p.number(808 + 8 * (64 + 6 - 1), 4);
    if (!p)
    {
        return Result<Battle>::failure(p.error());
    }
    if (raw < 15 || pal.size() < 768)
    {
        return Result<Battle>::failure(make_error("Invalid battle map table"));
    }
    size_t group = raw - 15;
    unsigned maps = r.number(group);
    if (!map || map > maps)
    {
        return Result<Battle>::failure(make_error("Invalid battle map ID"));
    }
    size_t m = group + r.number(group + 2 + size_t(map - 1) * 8, 4);
    if (!r.extent(m, 28))
    {
        return Result<Battle>::failure(r.error());
    }
    unsigned w = r.number(m), h = r.number(m + 2) * 2;
    if (!w || w > 256 || !h || h > 512)
    {
        return Result<Battle>::failure(make_error("Invalid battle map bounds"));
    }
    if (!r.extent(m + 28, size_t(w) * h * 5))
    {
        return Result<Battle>::failure(r.error());
    }
    std::vector<Cell> cells;
    for (size_t i = 0; i < size_t(w) * h; ++i)
    {
        size_t p = m + 28 + i * 5;
        cells.push_back({ int(prg.at(p)), int(word(prg, p + 1)), int(word(prg, p + 3)) });
    }
    Battle result(w, h, std::move(cells));
    unsigned rawSequences = p.number(808 + 8 * 64, 4);
    if (!p || rawSequences < 10)
    {
        return Result<Battle>::failure(make_error("Invalid action sequence table"));
    }
    size_t sequences = rawSequences - 10;
    unsigned appearances = r.number(sequences);
    if (!r.extent(sequences + 2, size_t(appearances) * 208))
    {
        return Result<Battle>::failure(r.error());
    }
    result.action_sequences_.resize(appearances);
    for (unsigned appearance = 0; appearance < appearances; ++appearance)
    {
        size_t cursor = sequences + 2 + appearances * 8 + appearance * 200, end = cursor + 200;
        for (auto& sequence : result.action_sequences_[appearance])
        {
            while (cursor < end && prg.at(cursor))
            {
                sequence.push_back(prg.at(cursor++));
            }
            if (cursor >= end)
            {
                break;
            }
            ++cursor;
        }
    }
    size_t special = m + 28 + size_t(w) * h * 5 + word(prg, m + 12) * 10;
    if (!r.extent(special, size_t(word(prg, m + 14)) * 6))
    {
        return Result<Battle>::failure(r.error());
    }
    for (unsigned i = 0; i < word(prg, m + 14); ++i)
    {
        size_t p = special + i * 6;
        if (word(prg, p + 4) & 0x8000)
        {
            result.sealed_cells_.push_back({ int(int16_t(word(prg, p))), int(int16_t(word(prg, p + 2))) });
        }
    }
    OriginalDiscResources disc_resources(root);
    auto datResult = disc_resources.try_dat_entry(disc, 5);
    if (!datResult)
    {
        return Result<Battle>::failure(datResult.error());
    }
    auto dat = std::move(datResult).value();
    ResourceReader icons(dat);
    if (!icons.extent(0, 2 + 75 * 8))
    {
        return Result<Battle>::failure(icons.error());
    }
    for (unsigned slot : { 15u, 17u, 19u, 18u, 25u, 26u, 27u, 28u, 29u, 30u, 31u, 32u, 35u, 36u, 37u, 59u, 60u, 61u, 62u, 63u, 64u, 65u, 66u, 67u, 68u, 69u, 70u, 71u, 72u, 73u, 74u })
    {
        size_t offset = icons.number(2 + slot * 8, 4), length = icons.number(6 + slot * 8, 4);
        if (!icons.extent(offset, length) || length < 4)
        {
            return Result<Battle>::failure(make_error("Invalid battle icon extent"));
        }
        size_t width = icons.number(offset) + 1, height = icons.number(offset + 2) + 1;
        if (width * height > length - 4)
        {
            return Result<Battle>::failure(make_error("Truncated battle icon"));
        }
    }
    unsigned rawAttributes = p.number(808 + 8 * (64 + 7 - 1), 4);
    if (!p || rawAttributes < 16)
    {
        return Result<Battle>::failure(make_error("Invalid battle attributes address"));
    }
    size_t attr_group = rawAttributes - 16, attributes = attr_group + 6;
    if (!r.extent(attr_group, 6))
    {
        return Result<Battle>::failure(r.error());
    }
    size_t attributeCount = word(prg, attr_group) + word(prg, attr_group + 2) + word(prg, attr_group + 4);
    if (!r.extent(attributes, attributeCount * 2))
    {
        return Result<Battle>::failure(r.error());
    }
    for (unsigned layer = 0; layer < 3; ++layer)
    {
        unsigned count = word(prg, attr_group + layer * 2);
        auto& dest = layer == 0 ? result.ground_attributes_ : result.object_attributes_;
        if (layer < 2)
        {
            for (unsigned i = 0; i < count; ++i)
            {
                dest.push_back(word(prg, attributes + i * 2));
            }
        }
        attributes += count * 2;
    }
    // 4296BF caches at most twenty category35 pads in map row order.
    for (unsigned y = 0; y < h; ++y)
    {
        for (unsigned x = 0; x < w - (y & 1); ++x)
        {
            if (result.object_category({ int(x), int(y) }) == 35 && result.teleport_pads_.size() < 20)
            {
                result.teleport_pads_.push_back({ int(x), int(y) });
            }
        }
    }
    auto movement = disc_resources.try_dat_entry(disc, 6);
    if (!movement)
    {
        return Result<Battle>::failure(movement.error());
    }
    result.movement_ = std::move(movement).value();
    // sub_4262EC loads ICONSO from DAT item 5; sub_4785E6 uses
    // pointer slots 35/36/37 = DAT entries 36/37/38 (slot 0 is entry 1).
    size_t effects = 0;
    for (unsigned i = 0; i < 3; ++i)
    {
        size_t p = effects + dword(dat, effects + 2 + (35 + i) * 8);
        if (word(dat, p) != 63 || word(dat, p + 2) != 31)
        {
            return Result<Battle>::failure(make_error("Invalid original movement effect"));
        }
        result.movement_effects_[i].assign(dat.begin() + p + 4, dat.begin() + p + 4 + 2048);
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        constexpr std::array<unsigned, 4> slots{ 15, 17, 19, 18 };
        size_t p = effects + dword(dat, effects + 2 + slots[i] * 8);
        unsigned width = word(dat, p) + 1, height = word(dat, p + 2) + 1;
        if (width > 64 || height > 64)
        {
            return Result<Battle>::failure(make_error("Invalid original movement mouse sprite"));
        }
        result.movement_mouse_sizes_[i] = { int(width), int(height) };
        result.movement_mouse_[i].assign(dat.begin() + p + 4, dat.begin() + p + 4 + size_t(width) * height);
    }
    for (unsigned i = 0; i < 16; ++i)
    {
        size_t p = effects + dword(dat, effects + 2 + (59 + i) * 8);
        auto& sprite = result.projectile59_[i];
        sprite.width = word(dat, p) + 1;
        sprite.height = word(dat, p + 2) + 1;
        if (size_t(sprite.width) * sprite.height > 0x5000)
        {
            return Result<Battle>::failure(make_error("Original projectile59 exceeds43E36B buffer"));
        }
        sprite.pixels.assign(dat.begin() + p + 4, dat.begin() + p + 4 + size_t(sprite.width) * sprite.height);
    }
    size_t ground_cursor = effects + dword(dat, effects + 2 + 25 * 8);
    if (word(dat, ground_cursor) != 63 || word(dat, ground_cursor + 2) != 31)
    {
        return Result<Battle>::failure(make_error("Invalid original ground cursor"));
    }
    result.ground_cursor_.assign(dat.begin() + ground_cursor + 4, dat.begin() + ground_cursor + 4 + 64 * 32);
    // sub_431347 / sub_447AFE compose ICONSO 27 and 28 (0x2244 bytes).
    for (unsigned i = 0; i < 2; ++i)
    {
        size_t cursor = effects + dword(dat, effects + 2 + (26 + i) * 8);
        if (word(dat, cursor) != 63 || word(dat, cursor + 2) != 136)
        {
            return Result<Battle>::failure(make_error("Invalid original selection frame"));
        }
        result.selection_frames_[i].assign(dat.begin() + cursor + 4, dat.begin() + cursor + 4 + 64 * 137);
    }
    // 4536E9/45386C use pointer slots 28..32, not selection frame slot 27.
    for (unsigned i = 0; i < 5; ++i)
    {
        size_t p = effects + dword(dat, effects + 2 + (28 + i) * 8);
        auto& sprite = result.finish_directions_[i];
        sprite.width = word(dat, p) + 1;
        sprite.height = word(dat, p + 2) + 1;
        if (size_t(sprite.width) * sprite.height > 0x5000)
        {
            return Result<Battle>::failure(make_error("Invalid original finish direction sprite"));
        }
        sprite.pixels.assign(dat.begin() + p + 4, dat.begin() + p + 4 + size_t(sprite.width) * sprite.height);
    }
    result.effect_palette_.assign(pal.begin(), pal.begin() + 768);
    result.effect_palette_[0] = result.effect_palette_[1] = result.effect_palette_[2] = 0;
    auto eventsResult = OriginalEvents::create(root, 1);
    if (!eventsResult)
    {
        return Result<Battle>::failure(eventsResult.error());
    }
    auto events = std::move(eventsResult).value();
    result.config_ = events.battle_config;
    result.original_rules_ = true;
    result.hostile_resources_ = std::make_shared<OriginalEvents>(events);
    auto equipment = OriginalPromotionTransaction::create(root);
    if (!equipment) { return Result<Battle>::failure(equipment.error()); }
    result.physical_equipment_ = std::make_shared<OriginalPromotionTransaction>(std::move(equipment).value());
    auto binResult = read_shared_resource(root / "SUPERSP.BIN"), kindsResult = read_shared_resource(root / "KINDSP.BIN");
    if (!binResult)
    {
        return Result<Battle>::failure(binResult.error());
    }
    if (!kindsResult)
    {
        return Result<Battle>::failure(kindsResult.error());
    }
    auto bin = std::move(binResult).value(), kinds = std::move(kindsResult).value();
    ResourceReader binReader(bin), kindReader(kinds);
    unsigned rawConfig = p.number(848 + 8 * 9, 4);
    if (!p || rawConfig < 20)
    {
        return Result<Battle>::failure(make_error("Invalid battle AI configuration address"));
    }
    size_t config = rawConfig - 20;
    config += 2 + 4 * size_t(binReader.number(config));
    config += 2 + binReader.number(config);
    config += 2;
    if (!binReader.extent(config, result.ai_config_.size()))
    {
        return Result<Battle>::failure(binReader.error());
    }
    unsigned kindCount = kindReader.number(22);
    size_t kindStart = kindReader.number(18, 4);
    if (!kindReader.extent(kindStart, size_t(kindCount) * 37))
    {
        return Result<Battle>::failure(kindReader.error());
    }
    for (size_t i = 0; i < result.ai_config_.size(); ++i)
    {
        result.ai_config_[i] = int(int8_t(bin.at(config + i)));
    }
    size_t kind_table = dword(kinds, 18);
    for (unsigned i = 0; i < word(kinds, 22); ++i)
    {
        result.ai_kind_classes_.push_back(int(int8_t(kinds.at(kind_table + size_t(i) * 37 + 17))));
        result.kind_movement_types_.push_back(int(int8_t(kinds.at(kind_table + size_t(i) * 37 + 18))));
    }
    return Result<Battle>::success(std::move(result));
}

auto Battle::load(const std::filesystem::path& root, unsigned map, OriginalDisc disc) -> Battle
{
    auto result = try_load(root, map, disc);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Battle::valid(Point p) const -> bool { return p.x >= 0 && p.y >= 0 && p.y < int(height_) && p.x < int(width_) - (p.y & 1); }

auto Battle::cell(Point p) const -> Cell { return valid(p) ? cells_.at(size_t(p.y) * width_ + p.x) : Cell{}; }

auto Battle::skill_area_cell(Point p) const -> bool
{
    if (!valid(p) || cell(p).ground <= 1) { return false; }
    const auto object = cell(p).object;
    return object <= 0 || size_t(object) > object_attributes_.size() || !(object_attributes_[size_t(object) - 1] & 0x8000);
}

auto Battle::spell_transfer_terrain(const OriginalEvents::Unit& unit, Point p) const -> bool
{
    if (!original_rules_ || !valid(p) || !unit.appearance || unit.appearance > kind_movement_types_.size() || cell(p).ground <= 0) { return false; }
    const auto target = cell(p);
    if (size_t(target.ground) > ground_attributes_.size()) { return false; }
    const unsigned attribute = ground_attributes_.at(size_t(target.ground - 1));
    const unsigned terrain = attribute >> 8;
    if (!attribute || terrain == 5) { return false; }
    const size_t stride = word(movement_, 0);
    const size_t kind = unit.appearance - 1;
    const size_t column = std::max(1u, attribute & 255) - 1;
    if (6 + (kind * stride + column) * 3 >= movement_.size() || int8_t(movement_.at(6 + (kind * stride + column) * 3)) == -1) { return false; }
    switch (kind_movement_types_[kind])
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 10:
        if (terrain != 0 && terrain != 1 && terrain != 2 && terrain != 4) { return false; }
        break;
    case 9:
        if (terrain != 1 && terrain != 2) { return false; }
        break;
    default: return false;
    }
    if (target.object)
    {
        if (target.object < 0 || size_t(target.object) > object_attributes_.size()) { return false; }
        const unsigned object = object_attributes_.at(size_t(target.object - 1));
        const unsigned category = object >> 8, local = object & 255;
        if (local > word(movement_, 4) || category == 1 || category == 2 || category == 10) { return false; }
        const size_t index = kind * stride + word(movement_, 2) - 1 + local;
        if (6 + index * 3 >= movement_.size() || int8_t(movement_.at(6 + index * 3)) == -1) { return false; }
    }
    return true;
}

auto Battle::spell_transfer_destinations(const OriginalEvents::Unit& caster, const OriginalEvents::Unit& transported, const std::vector<OriginalEvents::Unit>& units) const -> std::vector<Point>
{
    std::vector<Point> result;
    const Point source{ transported.x, transported.y };
    if (!valid(source)) { return result; }
    const int radius = std::clamp(int(int16_t(caster.sp)) / 10 + 1, 1, 10);
    int left = source.x, top = source.y;
    auto append = [&](Point p)
    {
        if (!valid(p) || !spell_transfer_terrain(transported, p) || std::abs(cell(source).elevation - cell(p).elevation) >= 10 || !skill_area_cell(p)) { return; }
        if ((!(p.y & 1) && p.x < 1) || p.x >= int(width_) - 1 || p.y >= int(height_) - 1 || p.y - std::max(0, cell(p).elevation - 1) < 1) { return; }
        for (const auto& other : units)
        {
            if (other.x == p.x && other.y == p.y && (other.status_flags & 0x80) && (!(other.status_flags & 0x20) || (other.status_flags & 2))) { return; }
        }
        result.push_back(p);
    };
    // 40295E: paired top/bottom edges, then paired left/right edges per ring.
    for (int ring = 1; ring <= radius; ++ring)
    {
        if (!(top & 1)) { --left; }
        --top;
        for (int x = left; x <= left + ring; ++x)
        {
            append({ x, top });
            append({ x, top + 2 * ring });
        }
        for (int y = top + 2; y <= top + 2 * ring - 2; y += 2)
        {
            append({ left, y });
            append({ left + ring, y });
        }
    }
    return result;
}

auto Battle::spell_summon_destination(const OriginalEvents::Unit& caster, unsigned appearance,
    const std::vector<OriginalEvents::Unit>& units) const -> std::optional<Point>
{
    const Point source{ caster.x, caster.y };
    if (!valid(source)) { return std::nullopt; }
    OriginalEvents::Unit summoned;
    summoned.appearance = appearance;
    int left = source.x;
    if (!(source.y & 1)) { --left; }
    const int top = source.y - 1;
    // 402E85 checks just ring1, paired upper/lower cells, and returns the first match.
    for (int x = left; x <= left + 1; ++x)
    {
        for (int y : { top, top + 2 })
        {
            const Point p{ x, y };
            if (!valid(p) || !spell_transfer_terrain(summoned, p) || !skill_area_cell(p)
                || std::abs(cell(source).elevation - cell(p).elevation) > 4) { continue; }
            if ((!(p.y & 1) && p.x < 1) || p.x >= int(width_) - 1 || p.y >= int(height_) - 1
                || p.y - std::max(0, cell(p).elevation - 1) < 1) { continue; }
            bool occupied = false;
            for (const auto& other : units)
            {
                if (other.x == p.x && other.y == p.y && (other.status_flags & 0x80)
                    && (!(other.status_flags & 0x20) || (other.status_flags & 2)))
                {
                    occupied = true;
                    break;
                }
            }
            if (!occupied) { return p; }
        }
    }
    return std::nullopt;
}

auto Battle::clear_runtime_object(Point p) -> void
{
    if (valid(p))
    {
        cells_.at(size_t(p.y) * width_ + p.x).object = 0;
    }
}

auto Battle::object_category(Point p) const -> unsigned
{
    auto object = cell(p).object;
    return object > 0 && size_t(object) <= object_attributes_.size() ? object_attributes_[size_t(object - 1)] >> 8 : 0;
}

auto Battle::object_cells(unsigned category) const -> std::vector<Point>
{
    std::vector<Point> result;
    for (int y = 0; y < int(height_); ++y)
    {
        for (int x = 0; x < int(width_) - (y & 1); ++x)
        {
            if (object_category({ x, y }) == category) { result.push_back({ x, y }); }
        }
    }
    return result;
}

auto Battle::settle_object_flags(OriginalEvents& events) const -> void
{
    for (const auto& u : events.units)
    {
        if (u.present(turn_) && (u.status_flags & 0x80) && !(u.status_flags & 0x20))
        {
            unsigned category = object_category({ u.x, u.y });
            if (!u.hostile() && u.runtime_role_selector != 1 && category == 4)
            {
                events.flags[7] = 1;
            }
            if (u.hostile() && category == 3)
            {
                events.flags[5] = 1;
            }
        }
    }
}

auto Battle::teleport_pads() const -> const std::vector<Point>& { return teleport_pads_; }

auto Battle::set_teleport_presenter(std::function<bool(unsigned, Point)> presenter) -> void { teleport_presenter_ = std::move(presenter); }

auto Battle::teleport_pending() const -> bool { return teleport_pending_; }

auto Battle::rollback_teleport() -> void
{
    if (teleport_random_baseline_)
    {
        random_ = *teleport_random_baseline_;
    }
    teleport_random_baseline_.reset();
    teleport_pending_ = false;
    teleport_movement_ = false;
    teleport_destination_.reset();
    teleport_cursor_ = 0;
}

auto Battle::try_finish_teleport() -> Result<bool>
{
    if (!teleport_pending_)
    {
        return Result<bool>::failure(make_error("No pending45DB10"));
    }
    teleport_pending_ = false;
    teleport_destination_.reset();
    teleport_random_baseline_.reset();
    if (!teleport_movement_) { ++teleport_cursor_; }
    teleport_movement_ = false;
    return Result<bool>::success(true);
}

auto Battle::finish_teleport() -> void
{
    auto result = try_finish_teleport();
    if (!result)
    {
        report_error(result.error());
    }
}

auto Battle::settle_teleport_pads(std::vector<OriginalEvents::Unit>& units, bool hostile) -> unsigned
{
    unsigned moved = 0;
    if (teleport_pending_)
    {
        return 0;
    }
    for (size_t index = teleport_cursor_; index < units.size(); ++index)
    {
        auto& actor = units[index];
        if (actor.hostile() != hostile || !(actor.status_flags & 0x80) || (actor.status_flags & 0x20) || object_category({ actor.x, actor.y }) != 35)
        {
            continue;
        }
        std::vector<Point> available;
        for (auto p : teleport_pads_)
        {
            if (occupant(units, p) < 0)
            {
                available.push_back(p);
            }
        }
        if (available.empty())
        {
            continue;
        }
        if (teleport_presenter_)
        {
            teleport_random_baseline_ = random_;
        }
        auto destination = available.at(size_t(roll(int(available.size()))));
        if (teleport_presenter_)
        {
            teleport_cursor_ = index;
            teleport_destination_ = destination;
            teleport_pending_ = true;
            if (!teleport_presenter_(unsigned(index), destination))
            {
                rollback_teleport();
                report_error(make_error("45DB10 original presentation unavailable"));
                return false;
            }
            return moved;
        }
        actor.x = destination.x;
        actor.y = destination.y;
        ++moved;
    }
    teleport_cursor_ = 0;
    return moved;
}

auto Battle::target_surface(Point p) const -> bool
{
    if (p.x < 0 || p.y < 0 || (!(p.y & 1) && p.x < 1) || p.x >= int(width_) - 1 || p.y >= int(height_) - 1)
    {
        return false;
    }
    int elevation = cell(p).elevation;
    return p.y - (elevation > 0 ? elevation - 1 : 0) >= 1 && std::find(sealed_cells_.begin(), sealed_cells_.end(), p) == sealed_cells_.end();
}

auto Battle::terrain_effect(const OriginalEvents& resources, const OriginalEvents::Unit& unit, Point p) const -> TerrainEffect
{
    TerrainEffect effect;
    if (!valid(p) || !unit.appearance || movement_.empty())
    {
        return effect;
    }
    auto tile = cell(p);
    if (!tile.ground)
    {
        return effect;
    }
    unsigned attr = ground_attributes_.at(size_t(tile.ground - 1));
    if (!attr)
    {
        return effect;
    }
    int terrain = int(int8_t(attr >> 8));
    unsigned local = attr & 255;
    size_t stride = word(movement_, 0), kind = unit.appearance - 1;
    auto triple = [&](size_t index)
    {
        size_t offset = 6 + index * 3;
        return std::array<uint8_t, 3>{ movement_.at(offset), movement_.at(offset + 1), movement_.at(offset + 2) };
    };
    effect.bytes = triple(kind * stride + (local ? local - 1 : 0));
    if (terrain == 5 || effect.bytes[0] == 255)
    {
        return effect;
    }
    bool special = resources.movement_class(unit) != 0;
    int type = unit.movement_type;
    if ((type >= 1 && type <= 5) || type == 10)
    {
        if (terrain == 0 || terrain == 1 || terrain == 2 || terrain == 4)
        {
            effect.result = 1;
        }
        else if (special)
        {
            effect.result = 2;
        }
        else
        {
            return effect;
        }
    }
    else if (type >= 6 && type <= 8)
    {
        effect.result = terrain == 2 || terrain == 3 ? 2 : 1;
    }
    else if (type == 9)
    {
        if (terrain == 1 || terrain == 2)
        {
            effect.result = 1;
        }
        else if (special)
        {
            effect.result = 2;
        }
        else
        {
            return effect;
        }
    }
    else
    {
        return effect;
    }
    if (tile.object)
    {
        unsigned object = object_attributes_.at(size_t(tile.object - 1)), category = object >> 8, object_local = object & 255;
        if (object_local > word(movement_, 4))
        {
            effect.result = 0;
            return effect;
        }
        auto extra = triple(kind * stride + word(movement_, 2) - 1 + object_local);
        if (category == 1 || extra[0] == 255)
        {
            effect.result = 0;
            return effect;
        }
        if (category == 2)
        {
            effect.result = 2;
        }
        else if (category == 10)
        {
            if (!special)
            {
                effect.result = 0;
                return effect;
            }
            effect.result = 2;
        }
        for (size_t i = 0; i < 3; ++i)
        {
            effect.bytes[i] = uint8_t(effect.bytes[i] + extra[i]);
        }
    }
    return effect;
}

auto Battle::target_visible(const OriginalEvents::Unit& target, std::span<const OriginalEvents::Unit> opposing, unsigned opposing_mode) const -> bool
{
    if (!(target.abilities & 0x80) && !(target.status3c & 0x10))
    {
        return true;
    }
    if (hostile_resources_ && hostile_resources_->army_modes)
    {
        opposing_mode |= (*hostile_resources_->army_modes)[target.hostile() ? 0 : 1];
    }
    if (opposing_mode & 9)
    {
        return true;
    }
    Point origin{ target.x, target.y };
    int height = cell(origin).elevation;
    const int right = origin.x + (origin.y & 1), left = right - 1;
    for (Point p : std::array<Point, 4>{ { { right, origin.y - 1 }, { left, origin.y + 1 }, { left, origin.y - 1 }, { right, origin.y + 1 } } })
    {
        auto observer = std::find_if(opposing.begin(), opposing.end(), [&](const auto& unit)
            {
                return (unit.status_flags & 0x80) && (!(unit.status_flags & 0x20) || (unit.status_flags & 2)) && unit.x == p.x && unit.y == p.y;
            });
        if (observer == opposing.end())
        {
            continue;
        }
        int other_height = cell(p).elevation, delta = other_height - height;
        if (other_height > 0 && delta >= -3 && delta <= 2)
        {
            return true;
        }
    }
    return false;
}

auto Battle::action_cells(Point origin, OriginalEvents::ActionRange range) const -> std::vector<Point>
{
    std::vector<Point> result;
    range.maximum = std::max(range.minimum, range.maximum);
    if (range.shape == 2)
    {
        Point top = origin;
        for (int radius = 1; radius <= range.maximum; ++radius)
        {
            if (!(top.y & 1))
            {
                --top.x;
            }
            --top.y;
            if (radius < range.minimum)
            {
                continue;
            }
            auto append = [&](Point p)
            {
                if (target_surface(p))
                {
                    result.push_back(p);
                }
            };
            for (int x = top.x; x <= top.x + radius; ++x)
            {
                append({ x, top.y });
                append({ x, top.y + 2 * radius });
            }
            for (int y = top.y + 2; y <= top.y + 2 * radius - 2; y += 2)
            {
                append({ top.x, y });
                append({ top.x + radius, y });
            }
        }
    }
    else
    {
        int height = cell(origin).elevation;
        for (int direction = 0; direction < 4; ++direction)
        {
            Point p = origin;
            for (int distance = 1; distance <= range.maximum; ++distance)
            {
                if (direction == 0)
                {
                    if (p.y & 1)
                    {
                        ++p.x;
                    }
                    --p.y;
                }
                else if (direction == 1)
                {
                    if (!(p.y & 1))
                    {
                        --p.x;
                    }
                    ++p.y;
                }
                else if (direction == 2)
                {
                    if (!(p.y & 1))
                    {
                        --p.x;
                    }
                    --p.y;
                }
                else
                {
                    if (p.y & 1)
                    {
                        ++p.x;
                    }
                    ++p.y;
                }
                if (distance < range.minimum)
                {
                    continue;
                }
                int delta = cell(p).elevation - height;
                if (!target_surface(p) || delta > 2 || delta < -3)
                {
                    break;
                }
                result.push_back(p);
            }
        }
    }
    return result;
}

auto Battle::attack_mode(Point attacker, Point defender, unsigned defender_direction, bool simple_rules) -> unsigned
{
    if (simple_rules)
    {
        return 1;
    }
    bool left = (attacker.y & 1) ? attacker.x < defender.x : attacker.x <= defender.x;
    unsigned int rear;
    if (left)
    {
        if (attacker.y > defender.y) { rear = 2u; }
        else
        {
            rear = 1u;
        }
    }
    else if (attacker.y > defender.y) { rear = 3u; }
    else
    {
        rear = 0u;
    }

    unsigned int front;
    if (left)
    {
        if (attacker.y > defender.y) { front = 0u; }
        else
        {
            front = 3u;
        }
    }
    else if (attacker.y > defender.y) { front = 1u; }
    else
    {
        front = 2u;
    }

    if (defender_direction == rear) { return 3; }
    else if (defender_direction == front) { return 1; }
    else
    {
        return 2;
    }
}

auto Battle::action_targets(const OriginalEvents& resources, const OriginalEvents::Unit& actor,
    const OriginalEvents::ActionRecord& action, std::span<const OriginalEvents::Unit> candidates,
    std::span<const OriginalEvents::Unit> opposing, unsigned opposing_mode) const -> std::vector<ActionTarget>
{
    std::vector<ActionTarget> result;
    if (!resources.action_equipment_allowed(actor, action))
    {
        return result;
    }
    for (Point p : action_cells({ actor.x, actor.y }, resources.action_range(actor, action)))
    {
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            const auto& candidate = candidates[i];
            if (!(candidate.status_flags & 0x80) || ((candidate.status_flags & 0x20) && !(candidate.status_flags & 2)) || candidate.x != p.x || candidate.y != p.y)
            {
                continue;
            }
            if (target_visible(candidate, opposing, opposing_mode))
            {
                result.push_back({ i, p, facing({ actor.x, actor.y }, p, actor.direction) });
            }
            break;
        }
    }
    return result;
}

auto Battle::counter_actions(const OriginalEvents& resources, const OriginalEvents::Unit& defender,
    Point attacker_position, std::span<const OriginalEvents::Unit> attackers, std::span<const OriginalEvents::Unit> defenders,
    unsigned attacker_mode, uint8_t defender_39, uint32_t defender_status3c_dword) const -> std::vector<unsigned>
{
    std::vector<unsigned> result;
    if ((defender_39 & 2) || (defender_status3c_dword & 0x2B000))
    {
        return result;
    }
    for (const auto& action : resources.action_records(defender))
    {
        if (action.fields[0] == 11 || action.fields[0] == 12)
        {
            continue;
        }
        auto targets = action_targets(resources, defender, action, attackers, defenders, attacker_mode);
        if (std::any_of(targets.begin(), targets.end(), [&](const auto& target)
                {
                    return target.position == attacker_position;
                }))
        {
            result.push_back(action.index);
        }
    }
    return result;
}

auto Battle::try_physical_target_group(const OriginalEvents& resources, const OriginalEvents::Unit& actor, const OriginalEvents::ActionRecord& action, const std::vector<OriginalEvents::Unit>& units, size_t primary) const -> Result<std::vector<size_t>>
{
    if (primary >= units.size()) { return Result<std::vector<size_t>>::failure(make_error("Original physical target index")); }
    auto range = resources.action_range(actor, action);
    auto cells = action_cells({ actor.x, actor.y }, range);
    auto selected = std::find(cells.begin(), cells.end(), Point{ units[primary].x, units[primary].y });
    if (selected == cells.end()) { return Result<std::vector<size_t>>::failure(make_error("Original physical target outside action ray")); }
    std::vector<size_t> result;
    if (range.shape != 1 && range.shape != 2)
    {
        // 406E7C stores each directional ray contiguously; 4074CE selects
        // the entire ray containing the primary, not every possible target.
        for (int direction = 0; direction < 4; ++direction)
        {
            Point position{ actor.x, actor.y };
            std::vector<Point> ray;
            for (int distance = 1; distance <= std::max(range.minimum, range.maximum); ++distance)
            {
                if (direction == 0)
                {
                    if (position.y & 1) { ++position.x; }
                    --position.y;
                }
                else if (direction == 1)
                {
                    if (!(position.y & 1)) { --position.x; }
                    ++position.y;
                }
                else if (direction == 2)
                {
                    if (!(position.y & 1)) { --position.x; }
                    --position.y;
                }
                else
                {
                    if (position.y & 1) { ++position.x; }
                    ++position.y;
                }
                if (distance < range.minimum) { continue; }
                if (std::find(cells.begin(), cells.end(), position) == cells.end()) { break; }
                ray.push_back(position);
            }
            if (std::find(ray.begin(), ray.end(), Point{ units[primary].x, units[primary].y }) == ray.end()) { continue; }
            for (Point position : ray)
            {
                auto occupant = std::find_if(units.begin(), units.end(), [&](const auto& unit)
                    {
                        return (unit.status_flags & 0x80) && (!(unit.status_flags & 0x20) || (unit.status_flags & 2)) && unit.x == position.x && unit.y == position.y;
                    });
                if (occupant == units.end()) { continue; }
                size_t index = size_t(occupant - units.begin());
                if (index == primary) { break; }
                result.push_back(index);
            }
            break;
        }
    }
    result.push_back(primary);
    return Result<std::vector<size_t>>::success(std::move(result));
}

auto Battle::physical_status(const OriginalEvents::Unit& unit, unsigned offset) -> uint8_t
{
    if (unit.action_status_runtime) { return (*unit.action_status_runtime)[offset]; }
    switch (offset)
    {
    case 0x39: return unit.status39.value_or(0);
    case 0x3C: return unit.status3c;
    case 0x3D: return unit.status3d;
    case 0x3E: return unit.status3e;
    case 0x3F: return unit.status3f;
    default: return 0;
    }
}

auto Battle::physical_status_dword(const OriginalEvents::Unit& unit) -> uint32_t
{
    uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) { value |= uint32_t(physical_status(unit, 0x3C + i)) << (8 * i); }
    return value;
}

auto Battle::try_physical_contexts(const OriginalEvents& resources, const std::vector<OriginalEvents::Unit>& units, size_t actor, size_t primary, const OriginalEvents::ActionRecord& action, bool reuse_primary) -> Result<std::vector<PhysicalActionContext>>
{
    if (actor >= units.size() || actor == primary) { return Result<std::vector<PhysicalActionContext>>::failure(make_error("Original physical actor index")); }
    auto group = try_physical_target_group(resources, units[actor], action, units, primary);
    if (!group) { return Result<std::vector<PhysicalActionContext>>::failure(group.error()); }
    std::vector<OriginalEvents::Unit> own, opponents;
    for (const auto& unit : units) { (unit.hostile() == units[actor].hostile() ? own : opponents).push_back(unit); }
    std::vector<PhysicalActionContext> contexts;
    auto staged_random = random_;
    for (size_t target : group.value())
    {
        PhysicalActionContext context{};
        context.actor = actor;
        context.target = target;
        context.actor_snapshot = units[actor];
        context.target_snapshot = units[target];
        context.action = action;
        context.mode = int(attack_mode({ units[actor].x, units[actor].y }, { units[target].x, units[target].y }, units[target].direction, resources.difficulty == 1));
        if (units[target].abilities & 0x2000) { context.mode = 1; }
        if (reuse_primary && target == primary)
        {
            contexts.push_back(std::move(context));
            continue;
        }
        auto records = resources.try_action_records(units[target]);
        if (!records) { return Result<std::vector<PhysicalActionContext>>::failure(records.error()); }
        std::vector<OriginalEvents::ActionRecord> pool;
        if (!(physical_status(units[target], 0x39) & 2) && !(physical_status_dword(units[target]) & 0x2B000))
        {
            for (const auto& candidate : records.value())
            {
                if (candidate.fields[0] == 11 || candidate.fields[0] == 12) { continue; }
                auto targets = action_targets(resources, units[target], candidate, own, opponents, 0);
                if (std::any_of(targets.begin(), targets.end(), [&](const auto& t)
                        {
                            return t.position == Point{ units[actor].x, units[actor].y };
                        }))
                {
                    pool.push_back(candidate);
                }
            }
        }
        if (!pool.empty() && !(reuse_primary && target == primary))
        {
            context.counter = pool[size_t(staged_random.bounded(int(pool.size()), random_seconds()))];
        }
        contexts.push_back(std::move(context));
    }
    random_ = std::move(staged_random);
    return Result<std::vector<PhysicalActionContext>>::success(std::move(contexts));
}

auto Battle::attack_availability(const OriginalEvents& resources, const OriginalEvents::Unit& actor,
    std::span<const OriginalEvents::Unit> candidates, std::span<const OriginalEvents::Unit> opposing, unsigned opposing_mode) const -> AttackAvailability
{
    AttackAvailability result;
    if (actor.action_points < attack_menu_cost(actor))
    {
        return result;
    }
    for (const auto& action : resources.action_records(actor))
    {
        int type = action.fields[0];
        if (type == 11 || type == 12)
        {
            continue;
        }
        if (action_targets(resources, actor, action, candidates, opposing, opposing_mode).empty())
        {
            continue;
        }
        int slot;
        if (type == 5) { slot = 1; }
        else if (type >= 1 && type <= 7) { slot = 0; }
        else if (type == 23) { slot = 6; }
        else if (type == 32) { slot = 5; }
        else if (type == 36) { slot = 4; }
        else
        {
            slot = 7;
        }

        result.mask |= attack_masks[slot];
        result.actions[slot] = int(action.index);
    }
    if (result.mask)
    {
        result.mask |= command_masks[0];
    }
    return result;
}

auto Battle::original_distance(Point from, Point to) -> int
{
    // 478AF3 scans staggered rings 1..79 and returns 80 when none matches.
    int left = from.x, top = from.y;
    for (int radius = 1; radius < 80; ++radius)
    {
        if (!(top & 1))
        {
            --left;
        }
        --top;
        if ((to.y == top || to.y == top + 2 * radius) && to.x >= left && to.x <= left + radius)
        {
            return radius;
        }
        if ((to.x == left || to.x == left + radius) && to.y > top && to.y < top + 2 * radius && ((to.y - top) & 1) == 0)
        {
            return radius;
        }
    }
    return 80;
}

auto Battle::adjacent(Point a, Point b) -> bool { return std::abs(a.y - b.y) == 1 && (b.x == a.x || b.x == a.x + ((a.y & 1) ? 1 : -1)); }

auto Battle::center(Point p) -> Point { return { p.x * 64 + (p.y & 1) * 32 + 32, p.y * 16 + 16 }; }

auto Battle::pick(float x, float y, int camera_x, int camera_y) const -> Point
{
    x += camera_x;
    y += camera_y;
    Point best{ -1, -1 };
    float distance = 2;
    int row = int(std::floor((y - 16) / 16));
    for (int yy = row - 2; yy <= row + 2; ++yy)
    {
        int col = int(std::floor((x - (yy & 1) * 32) / 64));
        for (int xx = col - 1; xx <= col + 1; ++xx)
        {
            Point p{ xx, yy };
            if (!valid(p))
            {
                continue;
            }
            auto c = center(p);
            float d = std::abs(x - c.x) / 32 + std::abs(y - c.y) / 16;
            if (d <= 1 && d < distance)
            {
                distance = d;
                best = p;
            }
        }
    }
    return best;
}

auto Battle::orient_finish_threat(const OriginalEvents& resources, std::vector<OriginalEvents::Unit>& units) -> bool
{
    if (state_ != State::finish_confirm || selected_ < 0)
    {
        return false;
    }
    auto& actor = units.at(size_t(selected_));
    std::vector<OriginalEvents::Unit> observers;
    for (const auto& unit : units)
    {
        if (unit.hostile() == actor.hostile() && unit.present(turn_))
        {
            observers.push_back(unit);
        }
    }
    std::vector<s2ne::MovementTarget> targets;
    for (size_t i = 0; i < units.size(); ++i)
    {
        const auto& unit = units[i];
        if (unit.hostile() == actor.hostile() || !unit.present(turn_))
        {
            continue;
        }
        targets.push_back({ i, { unit.x, unit.y }, unit.appearance != 0, unit.status_flags, 0, resources.combat_value(unit), !target_visible(unit, observers, 0) });
    }
    // 454159 -> 43948D(mode0): first nearest visible positive-value enemy.
    const auto* target = s2ne::original_target(targets, { actor.x, actor.y }, 0);
    if (!target)
    {
        return false;
    }
    const auto& enemy = units.at(target->token);
    const int divisor = resources.battle_config[4];    // config ECh, NOT movement cost E9h.
    if (!divisor)
    {
        report_error(make_error("Original finish threat division by zero"));
        return false;
    }
    const int reach = std::max(0, enemy.max_action_points / divisor) + resources.maximum_action_range(enemy);
    if (original_distance({ actor.x, actor.y }, { enemy.x, enemy.y }) > reach)
    {
        return false;
    }
    actor.direction = facing({ actor.x, actor.y }, { enemy.x, enemy.y }, actor.direction);
    actor.movement_frame = 0;
    actor.animation_action = 1;
    return true;
}

auto Battle::finish_action(std::vector<OriginalEvents::Unit>& units, bool confirmed) -> void
{
    if (state_ != State::finish_confirm || selected_ < 0)
    {
        return;
    }
    if (!confirmed)
    {
        state_ = State::command;
        return;
    }
    auto& unit = units.at(selected_);
    const bool can_pay_rest = original_rules_ && unit.action_points >= config_[8];
    if (can_pay_rest)
    {
        unit.action_points = int(int16_t(unit.action_points - config_[8]));
        unit.carry_points = std::min(unit.carry_points, unit.action_points);
        recover_rest(units, size_t(selected_));
    }
    idle_pose(unit);
    unit.status34 |= 0x80;
    unit.action_state = 2;
    selected_ = -1;
    state_ = State::idle;
    path_.clear();
    target_ = -1;
    message_ = "Unit action finished";
}

auto Battle::try_restore_turn(unsigned value) -> Result<bool>
{
    if (!value)
    {
        return Result<bool>::failure(make_error("Invalid saved battle turn"));
    }
    turn_ = value;
    hostile_side_ = false;
    selected_ = -1;
    state_ = State::idle;
    return Result<bool>::success(true);
}

auto Battle::restore_turn(unsigned value) -> void
{
    auto result = try_restore_turn(value);
    if (!result)
    {
        report_error(result.error());
    }
}

auto Battle::selected() const -> int { return selected_; }

auto Battle::player_controlled(const OriginalEvents::Unit& unit, size_t index) const -> bool
{
    return unit.controllable() || (selected_ == int(index) && !hostile_side_ && !hostile_actions_active_ && unit.behavior == 1 && (selected_context_ & 0x20));
}

auto Battle::set_auto_selection(bool enabled) -> void { auto_selection_ = enabled; }

auto Battle::player_selection_eligible(const OriginalEvents::Unit& unit, unsigned context_flags) const -> bool
{
    const bool allied_control = unit.controllable() || (unit.behavior == 1 && (context_flags & 0x20));
    if (!unit.present(turn_) || !allied_control || !(unit.status_flags & 0x80) || (unit.status_flags & 0x20) || unit.action_state == 2)
    {
        return false;
    }
    // 438EC3 -> 442B08 tests status, not remaining AP. A zero-AP unit
    // still enters the command loop and can explicitly finish its action.
    if (unit.action_status_runtime)
    {
        const auto& raw = *unit.action_status_runtime;
        uint32_t status = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            status |= uint32_t(raw[0x3C + i]) << (8 * i);
        }
        if ((raw[0x38] & 2) || (raw[0x39] & 2) || (status & 0x2B000))
        {
            return false;
        }
        if ((unsigned(raw[8]) | (unsigned(raw[9]) << 8)) == 1 && !(context_flags & 0x20))
        {
            return false;
        }
    }
    return valid({ unit.x, unit.y });
}

auto Battle::select_unit(std::vector<OriginalEvents::Unit>& units, size_t index, unsigned context_flags) -> bool
{
    if (hostile_side_ || hostile_actions_active_ || animating_movement() || combat_feedback_active() || state_ != State::idle || index >= units.size())
    {
        return false;
    }
    if (!player_selection_eligible(units[index], context_flags))
    {
        return false;
    }
    selected_ = int(index);
    selected_context_ = context_flags;
    before_move_ = units[index];
    moved_ = acted_ = false;
    command_ = 0;
    state_ = State::command;
    // 439A0E -> 447E59 enters action 1 before the command/movement loop.
    units[index].action_state = 1;
    if (units[index].animation_action != 1)
    {
        units[index].animation_action = 1;
        units[index].movement_frame = 0;
    }
    build_routes(units);
    prepare_commands(units);
    message_ = "Original command bar: available commands only";
    return true;
}

auto Battle::select_next_player_unit(std::vector<OriginalEvents::Unit>& units, bool restart, unsigned context_flags) -> bool
{
    if (restart)
    {
        next_player_unit_ = 0;
    }
    if (!auto_selection_ || units.empty() || state_ != State::idle || hostile_side_ || hostile_actions_active_ || animating_movement() || combat_feedback_active())
    {
        return false;
    }
    for (size_t offset = 0; offset < units.size(); ++offset)
    {
        size_t index = (next_player_unit_ + offset) % units.size();
        if (!player_selection_eligible(units[index], context_flags))
        {
            continue;
        }
        if (!select_unit(units, index, context_flags))
        {
            return false;
        }
        next_player_unit_ = (index + 1) % units.size();
        return true;
    }
    return false;
}

auto Battle::select_or_inspect_unit(std::vector<OriginalEvents::Unit>& units, size_t index, unsigned context_flags) -> bool
{
    if (hostile_side_ || hostile_actions_active_ || animating_movement() || combat_feedback_active() || state_ != State::idle || index >= units.size()) { return false; }
    const auto& unit = units[index];
    if (!unit.present(turn_) || !valid({ unit.x, unit.y }) || !(unit.status_flags & 0x80) || ((unit.status_flags & 0x20) && !(unit.status_flags & 2))) { return false; }
    if (unit.hostile())
    {
        std::vector<OriginalEvents::Unit> observers;
        for (const auto& observer : units)
        {
            if (!observer.hostile() && observer.present(turn_)) { observers.push_back(observer); }
        }
        if (!target_visible(unit, observers, context_flags)) { return false; }
    }
    if (select_unit(units, index, context_flags)) { return true; }
    requested_detail_unit_ = int(index);
    requested_command_ = 3;
    return true;
}

auto Battle::message() const -> const std::string& { return message_; }

auto Battle::state() const -> State { return state_; }

auto Battle::command_index() const -> int { return command_; }

auto Battle::target() const -> int { return target_; }

auto Battle::select_attack_target(const std::vector<OriginalEvents::Unit>& units, size_t index) -> bool
{
    if (state_ != State::attack_target || selected_ < 0 || index >= units.size() || !can_attack(units, size_t(selected_), index, attack_index_))
    {
        return false;
    }
    target_ = int(index);
    return true;
}

auto Battle::command_availability(const std::vector<OriginalEvents::Unit>& units) const -> uint32_t
{
    if (selected_ < 0 || size_t(selected_) >= units.size())
    {
        return 0;
    }
    const auto& u = units.at(selected_);
    if (!u.present(turn_) || !player_controlled(u, size_t(selected_)) || (original_rules_ && (!(u.status_flags & 0x80) || (u.status_flags & 0x20))))
    {
        return 0;
    }
    uint32_t mask = command_masks[5] | (original_rules_ ? command_masks[3] : 0);
    if ((!acted_ || original_rules_) && u.action_points >= (original_rules_ ? attack_menu_cost(u) : attack_cost(u)) && (!original_rules_ || attack_count(units) > 0))
    {
        mask |= command_masks[0];
    }
    const auto context = size_t(selected_) < movement_contexts_.size() ? movement_contexts_[selected_] : MovementContext{};
    const int minimum_move_ap = config_[0] + config_[context.movement_class ? 1 : 4];
    const bool has_move_ap = !original_rules_ || u.action_points >= minimum_move_ap;
    const bool already_moved = moved_ || (u.status34 & 1) != 0;
    if (!already_moved && has_move_ap)
    {
        if (original_rules_)
        {
            mask |= command_masks[1];
        }
        else
        {
            for (int cost : distances_)
            {
                if (cost != std::numeric_limits<int>::max() && cost > 0)
                {
                    mask |= command_masks[1];
                    break;
                }
            }
        }
    }
    if (command_request_available_)
    {
        for (int index : { 2, 4 })
        {
            if (command_request_available_(u, index))
            {
                mask |= command_masks[size_t(index)];
            }
        }
    }
    return mask;
}

auto Battle::refresh_command_availability(const std::vector<OriginalEvents::Unit>& units) -> void { available_mask_ = command_availability(units); }

auto Battle::command_available(const std::vector<OriginalEvents::Unit>& units, int index) const -> bool
{
    return index >= 0 && index < 6 && (command_availability(units) & command_masks[size_t(index)]) != 0;
}

auto Battle::command_slot(const std::vector<OriginalEvents::Unit>& units, int command) const -> int
{
    if (command < 0 || command >= 6 || !command_available(units, command))
    {
        return -1;
    }
    int slot = 0;
    for (int i = 0; i < command; ++i)
    {
        if (command_available(units, i))
        {
            ++slot;
        }
    }
    return slot;
}

auto Battle::command_from_slot(const std::vector<OriginalEvents::Unit>& units, int slot) const -> int
{
    if (slot < 0)
    {
        return -1;
    }
    for (int i = 0; i < 6; ++i)
    {
        if (command_available(units, i) && slot-- == 0)
        {
            return i;
        }
    }
    return -1;
}

auto Battle::command_count(const std::vector<OriginalEvents::Unit>& units) const -> int
{
    int count = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (command_available(units, i))
        {
            ++count;
        }
    }
    return count;
}

auto Battle::prepare_commands(const std::vector<OriginalEvents::Unit>& units) -> void
{
    available_mask_ = command_availability(units);
    if (command_ < 0 || command_ >= 6 || !command_available_for_cycle(command_))
    {
        command_ = command_from_slot(units, 0);
    }
}

auto Battle::route_costs() const -> const std::vector<int>& { return distances_; }

auto Battle::path() const -> const std::vector<Point>& { return path_; }

auto Battle::movement_category(int remaining, unsigned flags, int ability_threshold, int attack_threshold) -> uint8_t
{
    const bool allows_ability = (flags & 2) != 0;
    const bool allows_attack = (flags & 1) != 0;
    if (allows_ability && remaining >= ability_threshold)
    {
        return 2;
    }
    if (allows_attack && remaining >= attack_threshold)
    {
        return 1;
    }
    return 0;
}

auto Battle::set_movement_contexts(std::vector<MovementContext> contexts) -> void { movement_contexts_ = std::move(contexts); }

auto Battle::movement_categories() const -> const std::vector<uint8_t>& { return movement_categories_; }

auto Battle::set_movement_categories(const std::vector<uint8_t>& categories) -> void
{
    movement_categories_.assign(cells_.size(), 0);
    for (size_t i = 0; i < movement_categories_.size() && i < categories.size(); ++i)
    {
        movement_categories_[i] = categories[i] <= 2 ? categories[i] : 0;
    }
}

auto Battle::physical_exchange_diagnostics() const -> const std::vector<s2ne::Battle::PhysicalExchangeDiagnostic>& { return physical_exchange_diagnostics_; }

auto Battle::combat_feedback() const -> const s2ne::Feedback& { return combat_feedback_; }

auto Battle::combat_feedback_active() const -> bool { return rest_feedback_.has_value() || combat_feedback_.active() || skill94_death_lock_ || physical_exchange_.has_value(); }

auto Battle::lock_skill94_death(bool locked) -> void { skill94_death_lock_ = locked; }

auto Battle::feedback_presented() -> void
{
    if (rest_feedback_) { rest_feedback_->presented = true; }
    if (skill94_feedback_)
    {
        skill94_feedback_presented_ = true;
    }
}

auto Battle::begin_skill94_feedback(size_t actor, size_t target, int hp, int damage, std::function<void()> commit, unsigned effect, int caster_hp, int caster_max_hp, bool reverse, bool hit_flash) -> bool
{
    if (combat_feedback_active() || !commit || damage < 0 || damage > hp)
    {
        return false;
    }
    // 原46618D在471F51非零后先118／452E7F，再进入468BAC；MISS只显示数字。
    if (hit_flash)
    {
        return begin_curse33_feedback(actor, target, hp, true,
            [this, actor, target, hp, damage, commit = std::move(commit), effect, caster_hp, caster_max_hp, reverse]() mutable
            {
                if (!begin_skill94_feedback(actor, target, hp, damage, std::move(commit), effect, caster_hp, caster_max_hp, reverse))
                {
                    report_error(make_error("46618D post-flash feedback rejected"));
                }
            });
    }
    combat_feedback_.begin(actor, target, hp, damage, damage == 0, false);
    // 468BAC starts directly at number step0: no physical452E7F flash.
    combat_feedback_.phase = s2ne::Feedback::Phase::numbers;
    skill_feedback_effect_ = effect;
    skill_feedback_caster_hp_ = caster_hp;
    skill_feedback_caster_max_hp_ = caster_max_hp;
    skill_feedback_reverse_ = reverse;
    combat_feedback_.consume_with_caster(effect, skill_feedback_caster_hp_, caster_max_hp, reverse);
    skill94_feedback_ = true;
    skill94_feedback_presented_ = false;
    skill94_feedback_commit_ = std::move(commit);
    return true;
}

auto Battle::begin_healing_feedback(size_t actor, size_t target, int hp, int healing, std::function<void()> commit, bool sound) -> bool
{
    if (combat_feedback_active() || !commit || healing < 0) { return false; }
    if (sound && healing > 0 && effect_audio_) { effect_audio_(118, 0); }
    combat_feedback_.begin(actor, target, hp, -healing, healing == 0, false, hp + healing);
    combat_feedback_.phase = s2ne::Feedback::Phase::numbers;
    combat_feedback_.consume();
    skill_feedback_effect_ = 0;
    skill94_feedback_ = true;
    skill94_feedback_presented_ = false;
    skill94_feedback_commit_ = std::move(commit);
    return true;
}

auto Battle::begin_curse33_feedback(size_t actor, size_t target, int hp, bool hit, std::function<void()> commit) -> bool
{
    if (!begin_skill94_feedback(actor, target, hp, 0, std::move(commit))) { return false; }
    // 4677D7: miss goes to468BAC(0); success uses452E7F without a number.
    curse33_feedback_ = hit;
    if (hit && effect_audio_) { effect_audio_(118, 0); }
    if (hit) { combat_feedback_.phase = s2ne::Feedback::Phase::flash; }
    return true;
}

auto Battle::begin_status_area_feedback(size_t actor, std::vector<std::pair<size_t, bool>> targets,
    const std::vector<OriginalEvents::Unit>& units, std::function<void()> commit, bool blessing3, std::vector<int> amounts, bool healing) -> bool
{
    status_feedback_healing_ = healing;
    status_feedback_amounts_ = std::move(amounts);
    status_feedback_blessing3_ = blessing3;
    if (combat_feedback_active() || targets.empty() || !commit) { return false; }
    for (auto [i, hit] : targets)
    {
        if (i >= units.size()) { return false; }
    }
    status_feedback_actor_ = actor;
    status_feedback_cursor_ = 0;
    status_feedback_targets_ = std::move(targets);
    status_feedback_commit_ = std::move(commit);
    return next_status_feedback(units);
}

auto Battle::next_status_feedback(const std::vector<OriginalEvents::Unit>& units) -> bool
{
    auto [target, hit] = status_feedback_targets_[status_feedback_cursor_++];
    auto commit = [this, &units]
    {
        if (status_feedback_cursor_ < status_feedback_targets_.size())
        {
            next_status_feedback(units);
            return;
        }
        status_feedback_targets_.clear();
        status_feedback_cursor_ = 0;
        auto commit = std::move(status_feedback_commit_);
        commit();
    };
    if (status_feedback_blessing3_)
    {
        //46643F: an existing status extends its timer without feedback.
        if (!hit)
        {
            commit();
            return true;
        }
        if (!begin_healing_feedback(status_feedback_actor_, target, units[target].hp, status_feedback_amounts_.empty() ? 10 : status_feedback_amounts_.at(status_feedback_cursor_ - 1), std::move(commit))) { return false; }
        if (!status_feedback_healing_) { combat_feedback_.hp_after = units[target].hp; }
        return true;
    }
    return begin_curse33_feedback(status_feedback_actor_, target, units[target].hp, hit, std::move(commit));
}

auto Battle::set_death_settlement(std::function<void(std::vector<OriginalEvents::Unit>&, size_t, size_t)> callback) -> void { death_settlement_ = std::move(callback); }

auto Battle::set_physical_settlement(std::function<void(std::vector<OriginalEvents::Unit>&, const PhysicalSettlement&)> callback) -> void { physical_settlement_ = std::move(callback); }

auto Battle::set_physical_growth(std::function<void(std::vector<PhysicalSettlement>, std::function<void(bool)>)> callback) -> void { physical_growth_ = std::move(callback); }

auto Battle::set_physical_loot_finish(std::function<Result<bool>(bool)> callback) -> void { physical_loot_finish_ = std::move(callback); }

auto Battle::set_physical_loot_commit(std::function<Result<bool>(const std::vector<OriginalEvents::Unit>&, const std::vector<PhysicalSettlement>&)> callback) -> void { physical_loot_commit_ = std::move(callback); }

auto Battle::set_physical_death_presenter(std::function<Result<bool>(size_t, size_t)> callback, std::function<void(bool)> finish) -> void
{
    physical_death_presenter_ = std::move(callback);
    physical_transaction_finish_ = std::move(finish);
}

auto Battle::resume_physical_death() -> void
{
    if (physical_exchange_)
    {
        physical_exchange_->death.reset();
        physical_exchange_->death_started = false;
    }
}

auto Battle::apply_physical_action_status(std::vector<OriginalEvents::Unit>& units, size_t b, const OriginalEvents::ActionRecord& action, int result, std::optional<int> status_roll) -> int
{
    auto& target = units[b];
    s2ne::ActionStatusInput input{};
    input.result = result;
    int statusRoll;
    if (status_roll) { statusRoll = *status_roll; }
    else if (result > 0) { statusRoll = percent(); }
    else
    {
        statusRoll = 100;
    }

    input.roll = statusRoll;
    input.target_kind17 = hostile_resources_ ? hostile_resources_->action_status_kind(target) : 0;
    input.target_equipment14 = hostile_resources_ && hostile_resources_->equipment_special(target, 14);
    if (!target.action_status_runtime)
    {
        target.action_status_runtime.emplace();
        auto& bytes = *target.action_status_runtime;
        auto put = [&](size_t p, int value)
        {
            bytes[p] = uint8_t(value);
            bytes[p + 1] = uint8_t(unsigned(value) >> 8);
        };
        bytes[0x2F] = uint8_t(target.runtime_role_selector);
        bytes[0x33] = uint8_t(target.status_flags);
        bytes[0x39] = target.status39.value_or(0);
        bytes[0x3C] = target.status3c;
        bytes[0x3D] = target.status3d;
        bytes[0x3E] = target.status3e;
        bytes[0x3F] = target.status3f;
        bytes[0x116] = uint8_t(target.abilities >> 8);
        put(0xB5, target.level);
        put(0xB7, target.max_hp);
        put(0x124, target.hp);
        put(0xC3, target.attack);
        put(0xC5, target.defense);
        put(0x134, target.attack);
        put(0x136, target.defense);
    }
    auto& bytes = *target.action_status_runtime;
    bytes[0x33] = uint8_t(target.status_flags);
    if (!target.effect_state_5f8)
    {
        target.effect_state_5f8 = 0;
    }
    bytes[0x2F] = uint8_t(target.runtime_role_selector);
    auto refresh = [&](size_t p, int value)
    {
        bytes[p] = uint8_t(value);
        bytes[p + 1] = uint8_t(unsigned(value) >> 8);
    };
    refresh(0xB7, target.max_hp);
    refresh(0x124, target.hp);
    auto output = OriginalEvents::apply_action_record_status(bytes, action, input);
    target.attack = int(int16_t(unsigned(bytes[0x134]) | (unsigned(bytes[0x135]) << 8)));
    target.defense = int(int16_t(unsigned(bytes[0x136]) | (unsigned(bytes[0x137]) << 8)));
    return output.result;
}

auto Battle::apply_physical_status_tail(std::vector<OriginalEvents::Unit>& units, size_t actor, size_t target, const OriginalEvents::ActionRecord& action, int damage) -> int
{
    auto& victim = units[target];
    const auto& source = units[actor];
    if (damage <= 0 || (victim.status_flags & 0x20)) { return damage; }
    auto set_status = [&](unsigned offset, uint8_t value)
    {
        if (victim.action_status_runtime) { (*victim.action_status_runtime)[offset] = value; }
        if (offset == 0x3D) { victim.status3d = value; }
        if (offset == 0x3E) { victim.status3e = value; }
    };
    set_status(0x3E, physical_status(victim, 0x3E) & ~2);
    if ((physical_status(victim, 0x3D) & 0x80) && percent() <= 70) { set_status(0x3D, physical_status(victim, 0x3D) & 0x7F); }
    if ((physical_status(victim, 0x3E) & 0x10) || (victim.abilities & 0x4000)) { return damage; }
    auto proc = [&](int effect)
    {
        auto record = action;
        record.fields[17] = effect;
        record.fields[18] = 30;
        record.fields[19] = 3;
        apply_physical_action_status(units, target, record, damage);
    };
    if (action.fields[0] <= 10)
    {
        if ((hostile_resources_->equipment_special(source, 3) || (source.abilities & 0x80000)) && !(victim.abilities & 0x1000) && victim.runtime_role_selector != 1) { proc(1); }
        if (hostile_resources_->equipment_special(source, 7) && victim.runtime_role_selector != 1) { proc(2); }
        if ((source.abilities & 0x100000) && !(victim.runtime_role_selector & 3))
        {
            int threshold = hostile_resources_->equipment_special(victim, 14) ? 15 : 30;
            int draw = percent();
            if (draw <= threshold)
            {
                auto record = action;
                record.fields[17] = 0;
                apply_physical_action_status(units, target, record, damage, 100);
                s2ne::apply_timed_status(*victim.action_status_runtime, 0x4000, 3);
            }
        }
        if (source.abilities & 0x800000) { damage = -damage; }
    }
    if (source.abilities & 0x10000000)
    {
        if (victim.mp > 4) { units[actor].mp = std::min(units[actor].max_mp, units[actor].mp + victim.mp * 25 / 100); }
        if (victim.st > 4) { units[actor].st = std::min(units[actor].max_st, units[actor].st + victim.st * 25 / 100); }
    }
    return apply_physical_action_status(units, target, action, damage, percent());
}

auto Battle::settle_physical(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int lost) -> void
{
    auto damage_snapshot = units[b];
    damage_snapshot.hp += lost;
    if (physical_exchange_ && hostile_resources_)
    {
        auto& target = units[b];
        if (!target.hp && (hostile_resources_->equipment_special(target, 25) || hostile_resources_->equipment_special(target, 28)))
        {
            bool consume = !hostile_resources_->equipment_special(target, 25);
            target.hp = target.max_hp;
            if (consume)
            {
                auto slot = physical_equipment_->first_consumable_special(target, 28);
                OriginalPromotionTransaction::State state{ target, std::vector<uint8_t>(physical_equipment_->item_count()), OriginalRandom({ random_.seed_state(), random_.draw_count() }) };
                physical_equipment_->remove_equipment(state, *slot, true);
                target = std::move(state.unit);
            }
        }
        else if (target.hp > 0 && !physical_exchange_->reverse && hostile_resources_->equipment_special(target, 29) && percent() < 30)
        {
            target.hp = std::min(target.max_hp, target.hp + lost * 2);
        }
    }
    if (!units[b].hp)
    {
        units[b].status34 |= 0x80;
        if (physical_exchange_)
        {
            units[a].ending_kills = uint16_t(units[a].ending_kills + 1);
            units[b].ending_losses = uint16_t(units[b].ending_losses + 1);
        }
        if (physical_exchange_)
        {
            units[b].status_flags |= 0x20;
            physical_exchange_->deaths.emplace_back(a, b);
            if (physical_death_presenter_) { physical_exchange_->death = std::pair{ a, b }; }
        }
        else if (death_settlement_)
        {
            death_settlement_(units, a, b);
        }
    }
    if (!physical_settlement_ && !physical_exchange_)
    {
        return;
    }
    // 43CAD4 -> 409454 -> 4094C4, followed by 458B03. Skill469101
    // is a different chain. The kill-equipment459227 supplement is external.
    if (physical_exchange_ && !physical_exchange_->reverse)
    {
        physical_exchange_->forward_awards.push_back({ a, b, units[a].id, units[b].id, lost, 0, !units[b].hp });
        physical_exchange_->forward_targets.push_back(std::move(damage_snapshot));
        return;
    }
    int xp = 1;
    if (hostile_resources_ && (lost || (units[b].status_flags & 0x20)))
    {
        auto target_snapshot = damage_snapshot;    // 43EBCA saves HP in runtime132 before subtraction.
        auto actor_snapshot = units[a];
        actor_snapshot.ending_combat_value = actor_snapshot.ending_combat_value.value_or(actor_snapshot.hp);
        target_snapshot.ending_combat_value = target_snapshot.hp;
        int actor = hostile_resources_->combat_value(actor_snapshot, 1), target = hostile_resources_->combat_value(target_snapshot, 1);
        if (actor > 0 && actor + target > 0 && ai_config_[0xE1])
        {
            xp = std::max(1, (lost * target * ai_config_[0xE5] / (actor + target)) / ai_config_[0xE1]);
            if (!units[b].hp)
            {
                xp += ai_config_[0xE2] * target / actor + 3;
            }
        }
        else
        {
            return;    // No invented divisor or missing context fallback.
        }
    }
    if (units[b].status_flags & 0x20) { xp += int(units[b].physical_kill_xp); }
    PhysicalSettlement award{ a, b, units[a].id, units[b].id, lost, xp, !units[b].hp };
    if (physical_exchange_)
    {
        (physical_exchange_->reverse ? physical_exchange_->counter_awards : physical_exchange_->forward_awards).push_back(award);
        return;
    }
    physical_settlement_(units, award);
}

auto Battle::tick_combat_feedback(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds) -> void
{
    if (rest_feedback_)
    {
        auto& rest = *rest_feedback_;
        rest.elapsed += std::min(milliseconds, 1000u);
        // 原45455F：16次实际呈现，每次等待3个百分秒，不补跳帧。
        if (!rest.presented || rest.elapsed < 30) { return; }
        rest.elapsed = 0;
        rest.presented = false;
        if (++rest.step == 16) { rest_feedback_.reset(); }
        return;
    }
    auto& feedback = combat_feedback_;
    if (!feedback.active())
    {
        advance_physical_exchange(units);
        return;
    }
    if (skill94_feedback_)
    {
        feedback.elapsed += std::min(milliseconds, 1000u);
        if (curse33_feedback_)
        {
            if (!skill94_feedback_presented_ || feedback.elapsed < 20) { return; }
            feedback.phase = s2ne::Feedback::Phase::idle;
            curse33_feedback_ = skill94_feedback_ = skill94_feedback_presented_ = false;
            auto commit = std::move(skill94_feedback_commit_);
            commit();
            return;
        }
        unsigned int interval;
        if (feedback.step < 3) { interval = 30u; }
        else if (feedback.step < 4) { interval = 20u; }
        else
        {
            interval = 10u;
        }

        // 468F5A waits only for steps0..5, for both damage and MISS.
        if (!skill94_feedback_presented_ || (feedback.step < 6 && feedback.elapsed < interval))
        {
            return;
        }
        skill94_feedback_presented_ = false;
        feedback.elapsed = 0;
        ++feedback.step;
        if (feedback.step >= 8 && !feedback.remaining)
        {
            feedback.phase = s2ne::Feedback::Phase::idle;
            skill94_feedback_ = false;
            auto commit = std::move(skill94_feedback_commit_);
            commit();
            return;
        }
        feedback.consume_with_caster(skill_feedback_effect_, skill_feedback_caster_hp_, skill_feedback_caster_max_hp_, skill_feedback_reverse_);
        return;
    }
    if (feedback.phase == s2ne::Feedback::Phase::windup)
    {
        if (animating_movement())
        {
            return;
        }
        if (projectile_)
        {
            projectile_elapsed_ += std::min(milliseconds, 1000u);
            if (projectile_elapsed_ < 10)
            {
                return;
            }
            projectile_elapsed_ = 0;
            if (OriginalEvents::advance_effect_flight(*projectile_))
            {
                return;
            }
            projectile_.reset();
        }
        feedback.phase = s2ne::Feedback::Phase::flash;
        feedback.elapsed = 0;
        if (effect_audio_ && physical_sound_ > 0)
        {
            effect_audio_(unsigned(physical_sound_), 0);
            physical_sound_ = 0;
        }
        return;
    }
    feedback.elapsed += std::min(milliseconds, 1000u);
    if (feedback.phase == s2ne::Feedback::Phase::flash)
    {
        if (feedback.elapsed < 20)
        {
            return;
        }
        if (pending_action_status_)
        {
            auto action = *pending_action_status_;
            pending_action_status_.reset();
            int result = apply_physical_action_status(units, feedback.target, action, feedback.damage);
            feedback.begin(feedback.attacker, feedback.target, units[feedback.target].hp, result, feedback.miss, false);
        }
        feedback.phase = s2ne::Feedback::Phase::numbers;
        feedback.elapsed = 0;
        units.at(feedback.target).hp -= feedback.consume();
        return;
    }
    // Original polling resets the timestamp after every presented frame.
    if (feedback.elapsed < feedback.interval())
    {
        return;
    }
    feedback.elapsed = 0;
    ++feedback.step;
    if ((feedback.miss && feedback.step >= 7) || (!feedback.miss && feedback.step >= 8 && !feedback.remaining))
    {
        size_t a = feedback.attacker, b = feedback.target;
        feedback.phase = s2ne::Feedback::Phase::idle;
        settle_physical(units, a, b, feedback.damage);
        return;
    }
    units.at(feedback.target).hp -= feedback.consume();
}

auto Battle::enable_movement_presentation() -> void { movement_presentation_driven_ = true; }

auto Battle::movement_presented() -> void
{
    if (animating_movement()) { movement_presented_ = true; }
}

auto Battle::enable_movement_animation() -> void { movement_animation_enabled_ = true; }

auto Battle::animating_movement() const -> bool { return moving_unit_ >= 0; }

auto Battle::attack_animation_active() const -> bool { return attack_animation_ && animating_movement(); }

auto Battle::tick_movement(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds) -> void
{
    if (!animating_movement())
    {
        return;
    }
    // 452167 resets its clock after each presented pose. Discard frame
    // overruns instead of carrying them into (or skipping) the next pose.
    unsigned frame = movement_elapsed_ / movement_interval_;
    if (!attack_animation_ && movement_sound_frame_ != int(frame))
    {
        movement_sound(movement_poses_.at(frame));
        movement_sound_frame_ = int(frame);
    }
    unsigned elapsed = movement_elapsed_ % movement_interval_ + std::min(milliseconds, movement_interval_);
    if (elapsed >= movement_interval_)
    {
        if (movement_presentation_driven_ && !movement_presented_)
        {
            movement_elapsed_ = frame * movement_interval_ + movement_interval_ - 1;
            return;
        }
        movement_presented_ = false;
        ++frame;
        if (frame >= movement_poses_.size())
        {
            // Complete the route before an attack/reward can capture its unit.
            // Intermediate controller edges retain the walking frame sequence.
            // The original route completion leaves the final movement
            // cursor visible; command cancellation is responsible for
            // restoring the idle pose.
            moving_unit_ = -1;
            movement_sound_frame_ = -1;
            movement_sound_channel_ = movement_sound_id_ = 0;
            movement_poses_.clear();
            movement_elapsed_ = 0;
            attack_animation_ = false;
            return;
        }
        movement_elapsed_ = frame * movement_interval_;
        if (!attack_animation_)
        {
            movement_sound(movement_poses_.at(frame));
            movement_sound_frame_ = int(frame);
        }
    }
    else
    {
        movement_elapsed_ = frame * movement_interval_ + elapsed;
    }
}

auto Battle::render_units(const std::vector<OriginalEvents::Unit>& units) const -> std::vector<OriginalEvents::Unit>
{
    auto result = units;
    if (animating_movement() && size_t(moving_unit_) < result.size())
    {
        result[moving_unit_] = movement_poses_.at(movement_elapsed_ / movement_interval_);
    }
    // 43C43E only advances a successful target on the final two attack
    // frames (440618(cursor+2)==0), preserving its existing action/direction.
    if (reaction_pose_ && attack_animation_active() && combat_feedback_.target < result.size())
    {
        size_t frame = movement_elapsed_ / movement_interval_, count = movement_poses_.size();
        if (frame + 2 >= count)
        {
            auto& pose = result[combat_feedback_.target];
            pose = *reaction_pose_;
            unsigned advances = unsigned(frame - (count > 2 ? count - 2 : 0) + 1);
            unsigned sequence = pose.animation_action * 4 + (pose.direction & 3);
            if (pose.appearance && pose.appearance <= action_sequences_.size() && sequence < 30)
            {
                const auto& frames = action_sequences_[pose.appearance - 1][sequence];
                if (!frames.empty())
                {
                    pose.movement_pose = true;
                    pose.movement_frame = (pose.movement_frame + advances) % unsigned(frames.size());
                }
            }
        }
    }
    // Presentation-only presence keeps a lethal target visible until its HP
    // number sequence and settlement callback complete; gameplay stays dead.
    if (combat_feedback_active() && combat_feedback_.target < result.size())
    {
        result[combat_feedback_.target].hp = std::max(1, skill94_feedback_ ? combat_feedback_.hp_after : result[combat_feedback_.target].hp);
    }
    if (skill94_feedback_ && (skill_feedback_effect_ == 41 || skill_feedback_effect_ == 42) && combat_feedback_.attacker < result.size())
    {
        result[combat_feedback_.attacker].hp = skill_feedback_caster_hp_;
    }
    return result;
}

auto Battle::cancel() -> void
{
    if (animating_movement() || combat_feedback_active())
    {
        return;
    }
    path_.clear();
    target_ = -1;
    if (state_ == State::movement || state_ == State::attack_menu || state_ == State::attack_target || state_ == State::item_menu)
    {
        state_ = State::command;
        message_ = "Command cancelled; select another command";
        return;
    }
    selected_ = -1;
    state_ = State::idle;
    message_ = "Select a unit to open its original command bar";
}

auto Battle::cancel(std::vector<OriginalEvents::Unit>& units) -> void
{
    if (animating_movement() || combat_feedback_active())
    {
        return;
    }
    // 0x439E6D restores the saved position/AP when leaving a moved, uncommitted action.
    if (state_ == State::command && moved_ && !acted_ && selected_ >= 0)
    {
        units.at(selected_) = before_move_;
        moved_ = false;
        build_routes(units);
        prepare_commands(units);
        message_ = "Movement undone";
        return;
    }
    if (state_ == State::command && selected_ >= 0)
    {
        units.at(selected_).action_state = 0;
        idle_pose(units.at(selected_));
    }
    cancel();
}

auto Battle::surface_center(Point p, Cell c) -> Point
{
    auto q = center(p);
    q.y -= std::max(0, c.elevation - 1) * 16;
    return q;
}

auto Battle::screen_center(Point p, int cx, int cy) const -> Point
{
    auto q = surface_center(p, cell(p));
    return { q.x - cx, q.y - cy };
}

auto Battle::set_mask_surfaces(bool enabled) -> void { mask_surfaces_ = enabled; }

auto Battle::pick_surface(float x, float y, int cx, int cy) const -> Point
{
    const int horizontal = (x < 0 || x >= 640) ? int(std::floor(x / 64.0f)) * 64 : 0;
    const int vertical = (y < 0 || y >= 480) ? int(std::floor(y / 32.0f)) * 32 : 0;
    x -= horizontal;
    cx += horizontal;
    y -= vertical;
    cy += vertical;
    if (original_rules_)
    {
        // 42A2BD/42A3D2: integer division truncates toward zero, including negative edges.
        int half = cx / 32, camera_row = cy / 16;
        int mx = int(x) + ((half & 1) ? 32 : 0), my = int(y) + ((camera_row & 1) ? 16 : 0);
        int left = -((mx + 2 * my - 32) / 64) * 64 - 32;
        int right = ((2 * my - mx + 608) / 64) * 64 - 608;
        int anchor_x = (-left - right) / 2, anchor_y = (right - left) / 4;
        int local_x = mx - anchor_x + 32, local_y = my - anchor_y;
        int quadrant;
        if (2 * local_y + local_x < 64)
        {
            if (local_x - 2 * local_y < 0) { quadrant = 3; }
            else
            {
                quadrant = 1;
            }
        }
        else if (local_x - 2 * local_y < 0) { quadrant = 2; }
        else
        {
            quadrant = 4;
        }

        int col = (anchor_x - 32) / 64 + half / 2;
        if (anchor_x == 0 && !(half & 1))
        {
            --col;
        }
        int base_row = anchor_y / 16 + camera_row - ((camera_row & 1) ? 1 : 0);
        auto height_at = [&](Point p)
        {
            if (!valid(p) || p.y < 1 || (!(p.y & 1) && (p.x == 0 || p.x == int(width_) - 1)))
            {
                return 0;
            }
            return cell(p).elevation;
        };
        for (int rise = 20; rise >= 0; rise -= 2)
        {
            Point p{ col, base_row + rise };
            if (valid(p))
            {
                int h = height_at(p);
                if (h == rise + 1 || (h == rise && quadrant == 2))
                {
                    return p;
                }
                if (h == rise + 2 && quadrant == 1)
                {
                    return p;
                }
                if (!mask_surfaces_ && h >= rise + 2)
                {
                    return { -1, -1 };
                }
            }
            if (!rise)
            {
                continue;
            }
            p = { col - (((base_row + rise - 1) & 1) ? 1 : 0), base_row + rise - 1 };
            if (valid(p))
            {
                int h = height_at(p);
                if (h == rise && quadrant == 3)
                {
                    return p;
                }
                if (!mask_surfaces_ && local_x < 32 && ((h == rise && local_y >= 16) || h > rise))
                {
                    return { -1, -1 };
                }
            }
            p = { col + (((base_row + rise - 1) & 1) ? 0 : 1), base_row + rise - 1 };
            if (valid(p))
            {
                int h = height_at(p);
                if (h == rise && quadrant == 4)
                {
                    return p;
                }
                if (!mask_surfaces_ && local_x >= 32 && ((h == rise && local_y >= 16) || h > rise))
                {
                    return { -1, -1 };
                }
            }
        }
        return { -1, -1 };
    }
    Point best{ -1, -1 };
    // Synthetic test maps use the geometric surface without the original terrain picker.
    for (int row = 0; row < int(height_); ++row)
    {
        for (int col = 0; col < int(width_) - (row & 1); ++col)
        {
            Point p{ col, row };
            if (cell(p).elevation <= 0)
            {
                continue;
            }
            auto c = screen_center(p, cx, cy);
            if (std::abs(x - c.x) / 32 + std::abs(y - c.y) / 16 <= 1)
            {
                best = p;
            }
        }
    }
    return best;
}

auto Battle::command_origin(const std::vector<OriginalEvents::Unit>& units, int cx, int cy) const -> Point
{
    if (selected_ < 0)
    {
        return { -1, -1 };
    }
    auto c = screen_center({ units[selected_].x, units[selected_].y }, cx, cy);
    int count = state_ == State::attack_menu ? std::max(1, attack_count(units)) : std::max(1, command_count(units));
    if (state_ == State::item_menu) { count = 2; }
    const auto context = size_t(selected_) < movement_contexts_.size() ? movement_contexts_[selected_] : MovementContext{};
    int y = c.y + 16;
    if (context.movement_class >= 3 && context.movement_class <= 5)
    {
        y -= 64;
    }
    y = std::max(0, y);
    if (y > 330)
    {
        y -= int(context.sprite_height) + 72;
    }
    return { std::clamp(c.x - count * 16, 8, 639 - count * 32), y };
}

auto Battle::player_route_passable(const std::vector<OriginalEvents::Unit>& units, const OriginalEvents::Unit& actor, Point p) const -> bool
{
    int i = occupant(units, p);
    return i < 0 || (original_rules_ && units[size_t(i)].hostile() == actor.hostile());
}

auto Battle::build_routes(const std::vector<OriginalEvents::Unit>& units) -> void
{
    distances_.assign(cells_.size(), std::numeric_limits<int>::max());
    previous_.assign(cells_.size(), -1);
    movement_categories_.assign(cells_.size(), 0);
    path_.clear();
    occupied_route_destinations_.clear();
    for (const auto& unit : units)
    {
        Point p{ unit.x, unit.y };
        if (occupant(units, p) >= 0)
        {
            occupied_route_destinations_.push_back(p);
        }
    }
    if (selected_ < 0)
    {
        return;
    }
    const auto& u = units[selected_];
    int start = u.y * int(width_) + u.x;
    using Node = std::pair<int, int>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> todo;
    distances_[start] = 0;
    todo.push({ 0, start });
    int budget = original_rules_ ? std::min(110, u.action_points - config_[0]) : 110;
    while (!todo.empty())
    {
        auto [cost, index] = todo.top();
        todo.pop();
        if (cost != distances_[index])
        {
            continue;
        }
        Point p{ index % int(width_), index / int(width_) };
        for (int dy : { -1, 1 })
        {
            for (int dx : { 0, (p.y & 1) ? 1 : -1 })
            {
                Point q{ p.x + dx, p.y + dy };
                int step = step_cost(u, p, q);
                if (step < 0 || !player_route_passable(units, u, q) || cost + step > budget)
                {
                    continue;
                }
                int next = q.y * int(width_) + q.x;
                if (cost + step < distances_[next])
                {
                    distances_[next] = cost + step;
                    previous_[next] = index;
                    todo.push({ cost + step, next });
                }
            }
        }
    }
    if (original_rules_)
    {
        const auto context = size_t(selected_) < movement_contexts_.size() ? movement_contexts_[selected_] : MovementContext{};
        unsigned flags = 1u | context.extra_flags | (context.has_usable_ability ? 2u : 0u);
        int ability_threshold = config_[7] + ((context.status34 & 0x0c) ? config_[11] : 0);
        int attack_threshold = config_[6] - ((context.status118 & 0x20) ? 4 : 0);
        if ((context.status34 & 2) && !(context.status116 & 0x80))
        {
            attack_threshold += config_[11];
        }
        for (size_t i = 0; i < distances_.size(); ++i)
        {
            if (previous_[i] >= 0)
            {
                movement_categories_[i] = movement_category(budget - distances_[i], flags, ability_threshold, attack_threshold);
            }
        }
    }
}

auto Battle::preview(Point p) -> bool
{
    path_.clear();
    if (!valid(p) || previous_.empty())
    {
        return false;
    }
    int index = p.y * int(width_) + p.x;
    if (previous_[index] < 0 || std::find(occupied_route_destinations_.begin(), occupied_route_destinations_.end(), p) != occupied_route_destinations_.end())
    {
        return false;
    }
    while (previous_[index] >= 0)
    {
        path_.push_back({ index % int(width_), index / int(width_) });
        index = previous_[index];
    }
    std::reverse(path_.begin(), path_.end());
    return true;
}

auto Battle::attack_menu_cost(const OriginalEvents::Unit& unit) const -> int
{
    // 43B096 reads dwords at EB/F0 then SAR 24: the signed bytes are EE/F3.
    return attack_cost(unit);
}

auto Battle::attack_cost(const OriginalEvents::Unit& unit) const -> int
{
    const bool reduced_attack_cost = (unit.abilities & 0x20000000u) != 0;
    const bool already_attacked = (unit.status34 & 2) != 0;
    const bool waives_repeat_cost = (unit.abilities & 0x8000) != 0;
    int cost = config_[6] - (reduced_attack_cost ? 4 : 0);
    if (already_attacked && !waives_repeat_cost)
    {
        cost += config_[11];
    }
    return cost;
}

auto Battle::executable_attack_slot(const OriginalEvents::ActionRecord& action) -> int
{
    int type = action.fields[0];
    if (type == 5) { return 1; }
    else if (type >= 1 && type <= 7) { return 0; }
    else if (type >= 21) { return 7; }
    else
    {
        return -1;
    }
}

auto Battle::executable_action(const std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot) const -> std::optional<OriginalEvents::ActionRecord>
{
    if (!hostile_resources_ || a >= units.size() || b >= units.size() || a == b)
    {
        return std::nullopt;
    }
    const auto& u = units[a];
    const auto& v = units[b];
    if (!u.present(turn_) || !v.present(turn_) || u.hostile() == v.hostile() || (original_rules_ && (!(u.status_flags & 0x80) || (u.status_flags & 0x20) || u.action_points < attack_cost(u))))
    {
        return std::nullopt;
    }
    std::vector<OriginalEvents::Unit> own;
    for (const auto& unit : units)
    {
        if (unit.present(turn_) && unit.hostile() == u.hostile())
        {
            own.push_back(unit);
        }
    }
    std::array<std::optional<OriginalEvents::ActionRecord>, 8> chosen;
    std::array<bool, 8> reaches{};
    for (const auto& action : hostile_resources_->action_records(u))
    {
        int action_slot = executable_attack_slot(action);
        if (action_slot < 0 || (slot >= 0 && slot != action_slot))
        {
            continue;
        }
        bool target_reachable = !action_targets(*hostile_resources_, u, action, std::span<const OriginalEvents::Unit>(&v, 1), own, 0).empty();
        bool any_target = target_reachable;
        if (!any_target)
        {
            for (const auto& candidate : units)
            {
                if (!candidate.present(turn_) || candidate.hostile() == u.hostile())
                {
                    continue;
                }
                if (!action_targets(*hostile_resources_, u, action, std::span<const OriginalEvents::Unit>(&candidate, 1), own, 0).empty())
                {
                    any_target = true;
                    break;
                }
            }
        }
        if (any_target)
        {
            chosen[action_slot] = action;
            reaches[action_slot] = target_reachable;
        }
    }
    for (int index = 0; index < 8; ++index)
    {
        if (reaches[index])
        {
            return chosen[index];
        }
    }
    return std::nullopt;
}

auto Battle::can_attack(const std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot) const -> bool
{
    if (hostile_resources_)
    {
        return executable_action(units, a, b, slot).has_value();
    }
    if (slot > 0 || a >= units.size() || b >= units.size() || a == b)
    {
        return false;
    }
    const auto& u = units[a];
    const auto& v = units[b];
    return u.present(turn_) && v.present(turn_) && u.hostile() != v.hostile() && adjacent({ u.x, u.y }, { v.x, v.y }) && cell({ u.x, u.y }).elevation == cell({ v.x, v.y }).elevation && (!original_rules_ || u.action_points >= attack_cost(u));
}

auto Battle::attack_command_available(const std::vector<OriginalEvents::Unit>& units, int index) const -> bool
{
    if (index < 0 || index > 7 || selected_ < 0)
    {
        return false;
    }
    if (index == 2 && command_request_available_ && command_request_available_(units[size_t(selected_)], 6))
    {
        return true;
    }
    if (index == 3 && command_request_available_ && command_request_available_(units[size_t(selected_)], 7))
    {
        return true;
    }
    for (size_t i = 0; i < units.size(); ++i)
    {
        if (can_attack(units, size_t(selected_), i, index))
        {
            return true;
        }
    }
    return false;
}

auto Battle::attack_index() const -> int { return attack_index_; }

auto Battle::attack_count(const std::vector<OriginalEvents::Unit>& units) const -> int
{
    int count = 0;
    for (int i = 0; i < 8; ++i)
    {
        count += attack_command_available(units, i);
    }
    return count;
}

auto Battle::attack_from_slot(const std::vector<OriginalEvents::Unit>& units, int slot) const -> int
{
    if (slot < 0)
    {
        return -1;
    }
    for (int index = 0; index < 8; ++index)
    {
        if (attack_command_available(units, index) && slot-- == 0)
        {
            return index;
        }
    }
    return -1;
}

auto Battle::cycle_command(int delta) -> void
{
    if (state_ == State::item_menu)
    {
        item_choice_ ^= 1;
        return;
    }
    if (state_ == State::attack_menu)
    {
        for (int n = 0; n < 8; ++n)
        {
            attack_index_ = (attack_index_ + (delta < 0 ? 7 : 1)) % 8;
            if (attack_menu_mask_ & (1u << attack_index_))
            {
                break;
            }
        }
        return;
    }
    if (state_ != State::command)
    {
        return;
    }
    // The original strip contains only entries whose availability bit is
    // set; keyboard cycling therefore skips disabled commands.
    int next = command_;
    for (int i = 0; i < 6; ++i)
    {
        next = (next + (delta < 0 ? -1 : 1) + 6) % 6;
        if (command_available_for_cycle(next))
        {
            command_ = next;
            return;
        }
    }
}

auto Battle::command_available_for_cycle(int index) const -> bool
{
    return (command_masks[size_t(index)] & available_mask_) != 0;
}

auto Battle::item_choice() const -> int { return item_choice_; }

auto Battle::cycle_target(const std::vector<OriginalEvents::Unit>& units) -> void
{
    if (state_ != State::attack_target || units.empty())
    {
        return;
    }
    for (size_t offset = 1; offset <= units.size(); ++offset)
    {
        size_t next = (size_t(std::max(target_, 0)) + offset) % units.size();
        if (can_attack(units, selected_, next, attack_index_))
        {
            target_ = int(next);
            return;
        }
    }
}

auto Battle::activate(std::vector<OriginalEvents::Unit>& units) -> bool
{
    if (animating_movement() || combat_feedback_active())
    {
        return false;
    }
    if (selected_ < 0)
    {
        return false;
    }
    if (state_ == State::item_menu)
    {
        requested_command_ = item_choice_ ? 8 : 2;
        state_ = State::command;
        return true;
    }
    if (state_ == State::attack_menu)
    {
        if (!attack_command_available(units, attack_index_))
        {
            return false;
        }
        if (effect_audio_) { effect_audio_(100, 0); }
        if (attack_index_ == 2 && command_request_available_ && command_request_available_(units[size_t(selected_)], 6))
        {
            requested_command_ = 6;
            state_ = State::command;
            return true;
        }
        if (attack_index_ == 3 && command_request_available_ && command_request_available_(units[size_t(selected_)], 7))
        {
            requested_command_ = 7;
            state_ = State::command;
            return true;
        }
        state_ = State::attack_target;
        target_ = -1;
        for (size_t i = 0; i < units.size(); ++i)
        {
            if (can_attack(units, selected_, i, attack_index_))
            {
                target_ = int(i);
                break;
            }
        }
        if (target_ < 0)
        {
            state_ = State::command;
            message_ = "No supported original action target";
            return false;
        }
        message_ = "Select target; Enter/Space or left click confirms; Esc cancels";
        return true;
    }
    if (state_ == State::attack_target)
    {
        if (target_ < 0 || !attack(units, selected_, target_, attack_index_))
        {
            return false;
        }
        acted_ = true;
        state_ = State::command;
        target_ = -1;
        return true;
    }
    if (state_ != State::command)
    {
        return false;
    }
    prepare_commands(units);
    if (!command_available_for_cycle(command_))
    {
        message_ = "Command disabled by original availability flags";
        return false;
    }
    if (command_ == 0)
    {
        attack_menu_mask_ = 0;
        for (int i = 0; i < 8; ++i)
        {
            if (attack_command_available(units, i))
            {
                attack_menu_mask_ |= 1u << i;
            }
        }
        attack_index_ = attack_from_slot(units, 0);
        state_ = State::attack_menu;
        message_ = "Original contact/ranged action submenu";
        return true;
    }
    if (command_ == 1)
    {
        if (moved_)
        {
            message_ = "Movement already used; cancel to undo before committing";
            return false;
        }
        state_ = State::movement;
        build_routes(units);
        message_ = "Choose a reachable destination; hover previews route, left click moves";
        return true;
    }
    if (command_ == 5)
    {
        state_ = State::finish_confirm;
        return true;
    }
    if (command_ == 2)
    {
        item_choice_ = 0;
        state_ = State::item_menu;
        return true;
    }
    if (command_ == 3 || command_ == 4)
    {
        requested_command_ = command_;
        return true;
    }
    message_ = "Original command not implemented; no action performed";
    return false;
}

auto Battle::hover(const std::vector<OriginalEvents::Unit>& units, Point p) -> void
{
    hovered_cell_ = p;
    hovered_ = valid(p) && occupant(units, p) >= 0 ? p : Point{ -1, -1 };
    if (state_ == State::movement)
    {
        preview(p);
    }
    if (state_ == State::attack_target)
    {
        int hit = attack_target_at(units, p);
        if (hit >= 0)
        {
            target_ = hit;
        }
    }
}

auto Battle::attack_target_at(const std::vector<OriginalEvents::Unit>& units, Point p) const -> int
{
    // 407400 searches the action's target records by cell, not the first occupant.
    for (size_t i = 0; i < units.size(); ++i)
    {
        if (units[i].x == p.x && units[i].y == p.y && can_attack(units, selected_, i, attack_index_))
        {
            return int(i);
        }
    }
    return -1;
}

auto Battle::mouse(std::vector<OriginalEvents::Unit>& units, float x, float y, bool pressed, int cx, int cy, unsigned context_flags) -> bool
{
    mouse_position_ = { int(x), int(y) };
    if (animating_movement() || combat_feedback_active())
    {
        return false;
    }
    if (state_ == State::command || state_ == State::attack_menu || state_ == State::item_menu)
    {
        prepare_commands(units);
        auto o = command_origin(units, cx, cy);
        int count = state_ == State::attack_menu ? attack_count(units) : command_count(units);
        if (state_ == State::item_menu) { count = 2; }
        if (x >= o.x && x < o.x + count * 32 && y >= o.y && y < o.y + 32)
        {
            if (state_ == State::item_menu) { item_choice_ = int(x - o.x) / 32; }
            else if (state_ == State::command)
            {
                int slot = int(x - o.x) / 32;
                int command = command_from_slot(units, slot);
                if (command < 0)
                {
                    return false;
                }
                command_ = command;
            }
            else
            {
                int action = attack_from_slot(units, int(x - o.x) / 32);
                if (action < 0)
                {
                    return false;
                }
                attack_index_ = action;
            }
            return pressed ? activate(units) : false;
        }
        return false;
    }
    auto p = pick_surface(x, y, cx, cy);
    hover(units, p);
    if (!pressed)
    {
        return false;
    }
    if (state_ == State::idle)
    {
        int hit = occupant(units, p);
        return hit >= 0 && select_or_inspect_unit(units, size_t(hit), context_flags);
    }
    return click(units, p);
}

auto Battle::occupant(const std::vector<OriginalEvents::Unit>& units, Point p) const -> int
{
    // 446E53 mode0: inactive roles never occupy a tile; settled dead
    // roles only remain spatially represented when status33 bit2 is set.
    for (size_t i = 0; i < units.size(); ++i)
    {
        if (units[i].present(turn_) && (!original_rules_ || ((units[i].status_flags & 0x80) && (!(units[i].status_flags & 0x20) || (units[i].status_flags & 2)))) && units[i].x == p.x && units[i].y == p.y)
        {
            return int(i);
        }
    }
    return -1;
}

auto Battle::observed_movement_edges(const std::vector<OriginalEvents::Unit>& units, size_t actor, bool occupied) const -> std::vector<std::array<int, 5>>
{
    std::vector<std::array<int, 5>> edges;
    if (actor >= units.size()) { return edges; }
    const auto& unit = units[actor];
    for (unsigned y = 0; y < height_; ++y)
    {
        for (unsigned x = 0; x < width_; ++x)
        {
            Point from{ int(x), int(y) };
            for (int dy : { -1, 1 })
            {
                for (int dx : { 0, (y & 1) ? 1 : -1 })
                {
                    Point to{ int(x) + dx, int(y) + dy };
                    int cost = step_cost(unit, from, to);
                    if (cost < 0 || (occupied && !player_route_passable(units, unit, to))) { continue; }
                    edges.push_back({ from.x, from.y, to.x, to.y, cost });
                }
            }
        }
    }
    return edges;
}

auto Battle::step_cost(const OriginalEvents::Unit& u, Point from, Point to) const -> int
{
    if (!valid(to) || !adjacent(from, to))
    {
        return -1;
    }
    auto a = cell(from), b = cell(to);
    if (!original_rules_)
    {
        return a.elevation > 0 && a.elevation == b.elevation && a.ground > 4 && b.ground > 4 && !b.object ? 1 : -1;
    }
    if (std::find(sealed_cells_.begin(), sealed_cells_.end(), to) != sealed_cells_.end())
    {
        return -1;
    }
    if (to.y < 1 || to.y >= int(height_) - 1 || (!(to.y & 1) && (to.x == 0 || to.x == int(width_) - 1)))
    {
        return -1;
    }
    int climb = b.elevation - a.elevation;
    const unsigned movementClass = hostile_resources_ ? hostile_resources_->movement_class(u) : 0;
    if (original_rules_ && hostile_resources_)
    {
        if (std::abs(climb) > 10 || to.y - b.elevation < 0) { return -1; }
        const bool ignores_slope = (physical_status(u, 0x117) & 1) || (physical_status(u, 0x3D) & 4);
        if (!ignores_slope && movementClass == 0 && (climb > 3 || climb < -4)) { return -1; }
        auto effect = terrain_effect(*hostile_resources_, u, to);
        if (!effect.result) { return -1; }
        const int uphill = ignores_slope || movementClass == 5 ? 0 : std::max(0, climb);
        if (movementClass != 0) { return ai_config_[0xE9] + ai_config_[0xEA] * uphill; }
        // 原477F1F在地表/物件BYTE相加后才符号扩展，不分别拒绝两个分量。
        return int(int8_t(effect.bytes[0])) + ai_config_[0xEB] * uphill;
    }
    if (climb > 3 || climb < -4 || to.y - b.elevation < 0 || b.ground <= 0)
    {
        return -1;
    }
    unsigned attr = ground_attributes_.at(b.ground - 1), terrain = attr >> 8;
    if (!attr || terrain == 5)
    {
        return -1;
    }
    int type = u.movement_type;
    if ((type >= 1 && type <= 5 || type == 10) && terrain != 0 && terrain != 1 && terrain != 2 && terrain != 4)
    {
        return -1;
    }
    if (type == 9 && terrain != 1 && terrain != 2)
    {
        return -1;
    }
    if (type < 1 || type > 10 || type == 7 || type == 8)
    {
        return -1;    // Flying modes require their separate height rules.
    }
    size_t stride = word(movement_, 0), kind = u.appearance - 1;
    size_t index = kind * stride + ((attr & 255) ? (attr & 255) - 1 : 0);
    int cost = int(int8_t(movement_.at(6 + index * 3)));
    if (cost < 0)
    {
        return -1;
    }
    if (b.object)
    {
        unsigned object = object_attributes_.at(b.object - 1), category = object >> 8, local = object & 255;
        if (category == 1 || category == 10 || local > word(movement_, 4))
        {
            return -1;
        }
        index = kind * stride + word(movement_, 2) - 1 + local;
        int extra = int(int8_t(movement_.at(6 + index * 3)));
        if (extra < 0)
        {
            return -1;
        }
        cost += extra;
    }
    return cost + config_[3] * std::max(0, climb);
}

auto Battle::can_step(const std::vector<OriginalEvents::Unit>& units, size_t index, Point to) const -> bool
{
    if (index >= units.size() || !units[index].present(turn_) || !player_controlled(units[index], index) || (original_rules_ && (!(units[index].status_flags & 0x80) || (units[index].status_flags & 0x20))))
    {
        return false;
    }
    const auto& u = units[index];
    int cost = step_cost(u, { u.x, u.y }, to);
    return cost >= 0 && occupant(units, to) < 0 && (!original_rules_ || cost <= std::min(110, u.action_points - config_[0]));
}

auto Battle::scripted_route(const OriginalEvents::Unit& unit, Point destination) const -> std::optional<std::vector<Point>>
{
    if (!original_rules_ || !valid(destination) || unit.movement_type < 1 || unit.movement_type > 10)
    {
        return std::nullopt;
    }
    auto actor = unit;
    std::vector<Point> path;
    for (unsigned chunk = 0; chunk < 20; ++chunk)
    {
        Position origin{ actor.x, actor.y };
        if (origin == Position{ destination.x, destination.y })
        {
            return path;
        }
        auto routes = build_ordered_routes(origin, [&](Position a, Position b) -> std::optional<int>
            {
                int cost = step_cost(actor, { a.x, a.y }, { b.x, b.y });
                return cost < 0 ? std::nullopt : std::optional<int>(cost);
            },
            [](Position)
            {
                return 1;
            },
            [&]
            {
                return int(int16_t(actor.action_points - config_[0]));
            },
            [](int)
            {
                return 0u;
            });
        int chosen = -1, best = 0, square = 0;
        for (size_t slot = 1; slot < routes.order.size(); ++slot)
        {
            int i = routes.order[slot];
            auto p = routes.records[i].route.position;
            if (p == Position{ destination.x, destination.y })
            {
                chosen = i;
                break;
            }
        }
        if (chosen < 0)
        {
            for (size_t slot = 1; slot < routes.order.size(); ++slot)
            {
                int i = routes.order[slot];
                auto p = routes.records[i].route.position;
                auto tile = cell({ p.x, p.y });
                unsigned terrain = tile.ground > 0 ? ground_attributes_.at(tile.ground - 1) >> 8 : 0;
                if (terrain == 1 || terrain == 2)
                {
                    continue;
                }
                int distance = s2ne::original_distance({ destination.x, destination.y }, p), s = (destination.x - p.x) * (destination.x - p.x) + (destination.y - p.y) * (destination.y - p.y);
                if (chosen < 0 || distance < best || (distance == best && s < square))
                {
                    chosen = i;
                    best = distance;
                    square = s;
                    if (best == 1)
                    {
                        break;
                    }
                }
            }
        }
        if (chosen < 0)
        {
            return std::nullopt;
        }
        std::vector<Point> segment;
        for (int node = chosen; node > 0; node = routes.records.at(node).parent)
        {
            auto p = routes.records.at(node).route.position;
            segment.push_back({ p.x, p.y });
        }
        if (segment.empty())
        {
            return std::nullopt;
        }
        std::reverse(segment.begin(), segment.end());
        path.insert(path.end(), segment.begin(), segment.end());
        actor.x = path.back().x;
        actor.y = path.back().y;
    }
    return std::nullopt;
}

auto Battle::scripted_frame_count(const OriginalEvents::Unit& unit, unsigned facing) const -> unsigned
{
    if (!unit.appearance || unit.appearance > action_sequences_.size())
    {
        return 0;
    }
    return unsigned(action_sequences_[unit.appearance - 1][4 + (facing & 3)].size());
}

auto Battle::hovered_unit(const std::vector<OriginalEvents::Unit>& units) const -> int { return valid(hovered_) ? occupant(units, hovered_) : -1; }

auto Battle::hovered_cell() const -> Point { return valid(hovered_cell_) ? hovered_cell_ : Point{ -1, -1 }; }

auto Battle::facing(Point from, Point to, unsigned current) -> unsigned
{
    if (from.x == to.x && from.y == to.y)
    {
        return current;
    }
    bool right = (from.y & 1) ? from.x < to.x : from.x <= to.x;
    if (right)
    {
        if (from.y > to.y) { return 2u; }
        else
        {
            return 1u;
        }
    }
    else if (from.y > to.y) { return 3u; }
    else
    {
        return 0u;
    }
}

auto Battle::step(std::vector<OriginalEvents::Unit>& units, size_t index, Point to, bool allied_transit) -> bool
{
    if (!can_step(units, index, to))
    {
        if (!allied_transit || index >= units.size() || !player_controlled(units[index], index) || !units[index].present(turn_) || !(units[index].status_flags & 0x80) || (units[index].status_flags & 0x20) || !player_route_passable(units, units[index], to))
        {
            return false;
        }
        int cost = step_cost(units[index], { units[index].x, units[index].y }, to);
        if (cost < 0 || cost > std::min(110, units[index].action_points - config_[0]))
        {
            return false;
        }
    }
    auto& u = units[index];
    if (original_rules_)
    {
        u.action_points -= step_cost(u, { u.x, u.y }, to);
        u.max_action_points = u.action_points;
        u.carry_points = std::min(u.carry_points, u.action_points);
    }
    u.direction = facing({ u.x, u.y }, to, u.direction);
    u.x = to.x;
    u.y = to.y;
    message_ = "Moved; action points " + std::to_string(u.action_points) + " (Enter: finish side)";
    return true;
}

auto Battle::ordinary_damage(int attack, int defense, int divisor, int minimum_roll) -> int
{
    return s2ne::ordinary_damage(attack, defense, divisor, minimum_roll);
}

auto Battle::seed(unsigned value) -> void
{
    random_seconds_ = value;
    random_ = s2ne::Random(1);
    random_.initialize(value);
}

auto Battle::mutate_combat_random(const std::function<bool(s2ne::Random&, uint32_t)>& transaction) -> bool
{
    auto staged = random_;
    if (!transaction(staged, random_seconds()))
    {
        return false;
    }
    random_ = staged;
    return true;
}

auto Battle::present_skill32_preparation(std::vector<OriginalEvents::Unit>& units, size_t index, size_t target, bool second) -> bool
{
    if (!movement_animation_enabled_ || index >= units.size() || target >= units.size())
    {
        return false;
    }
    auto& actor = units[index];
    if (!actor.appearance || actor.appearance > action_sequences_.size() || (actor.status_flags & 0x10))
    {
        return false;
    }
    if (!hostile_resources_)
    {
        return false;
    }
    auto actions = hostile_resources_->action_records(actor);
    auto casting = std::find_if(actions.begin(), actions.end(), [second](const auto& a)
        {
            return a.fields[0] == (second ? 12 : 11);
        });
    // 46C078 defaults to action offset8, overridden by signed action-record +Ah *4.
    int offset = casting == actions.end() ? 8 : int(int8_t(casting->fields[10])) * 4;
    if (offset < 0 || offset % 4 || offset >= 28)
    {
        return false;
    }
    const auto& destination = units[target];
    unsigned facing = actor.direction & 3;
    if (actor.x != destination.x || actor.y != destination.y)
    {
        bool right = (actor.y & 1) ? actor.x < destination.x : actor.x <= destination.x;
        unsigned int destinationFacing;
        if (right)
        {
            if (actor.y > destination.y) { destinationFacing = 2u; }
            else
            {
                destinationFacing = 1u;
            }
        }
        else if (actor.y > destination.y) { destinationFacing = 3u; }
        else
        {
            destinationFacing = 0u;
        }

        facing = destinationFacing;
    }
    unsigned sequence = unsigned(offset) + facing;
    const auto& frames = action_sequences_[actor.appearance - 1][sequence];
    if (frames.empty())
    {
        return false;
    }
    movement_poses_.clear();
    for (size_t frame = 0; frame < frames.size(); ++frame)
    {
        auto pose = actor;
        pose.direction = facing;
        pose.animation_action = unsigned(offset) / 4;
        pose.movement_pose = true;
        pose.movement_frame = unsigned(frame);
        movement_poses_.push_back(pose);
    }
    actor.direction = facing;
    // 454E43 ->473192 waits 0Ch DOS hundredths after each presented frame.
    moving_unit_ = int(index);
    movement_presented_ = false;
    movement_elapsed_ = 0;
    movement_interval_ = 12 * 10;
    attack_animation_ = true;
    if (effect_audio_) { effect_audio_(second ? 99u : 98u, 0); }
    return true;
}

auto Battle::mutate_random(const std::function<bool(OriginalRandom&, uint32_t)>& transaction) -> bool
{
    OriginalRandom staged({ random_.seed_state(), random_.draw_count() });
    if (!transaction(staged, random_seconds()))
    {
        return false;
    }
    auto state = staged.state();
    random_.restore_stream(state.seed, state.calls);
    return true;
}

auto Battle::preview_action_weight(const OriginalEvents& resources, const OriginalEvents::Unit& actor,
    const OriginalEvents::Unit& target, const OriginalEvents::ActionRecord& action) -> bool
{
    // 原40883D是附加效果价值，不是4096F3实击连击概率；预估也保留原抽签顺序。
    if ((physical_status(target, 0x3E) & 0x10) || (target.abilities & 0x4000)) { return false; }
    if (action.fields[0] <= 10)
    {
        int value = 0;
        if ((resources.equipment_special(actor, 3) || (target.abilities & 0x80000)) && !(target.abilities & 0x1000) && target.runtime_role_selector != 1)
        {
            value = 30;
        }
        if (!value && resources.equipment_special(actor, 7) && target.runtime_role_selector != 1) { value = 30; }
        if (!value && (actor.abilities & 0x100000) && !(target.runtime_role_selector & 3)) { value = 30; }
        if (actor.abilities & 0x800000) { return true; }
        if (resources.equipment_special(target, 14)) { value /= 2; }
        if (value > 30 && percent() > 50) { return true; }
    }
    if (!action.fields[17]) { return false; }
    int value = action.fields[18];
    if (resources.equipment_special(target, 14)) { value /= 2; }
    if ((target.runtime_role_selector & 3) && action.fields[17] == 8) { value = value * 7 / 10; }
    if (value < 30 || percent() < 50) { return false; }
    auto flags = physical_status_dword(target);
    switch (action.fields[17])
    {
    case 1: return !(target.abilities & 0x1000) && !(physical_status(target, 0x39) & 4) && target.runtime_role_selector != 1;
    case 2: return !(physical_status(target, 0x39) & 2) && !(flags & 0x2A000) && target.runtime_role_selector != 1;
    case 3: return !(physical_status(target, 0x39) & 2) && !(target.runtime_role_selector & 3);
    case 4: return !(physical_status(target, 0x39) & 2) && !(flags & 0x2B000) && target.runtime_role_selector != 1;
    // 原408B42取45CFBB类别2的分量1/3；不是结算快照类别1或不存在的类别3。
    case 5: return !(physical_status(target, 0x38) & 2) && !(physical_status(target, 0x39) & 2) && !(flags & 0x2B000) && resources.combat_value(target, 2, 1) > 2 * resources.combat_value(target, 2, 3);
    case 6: return target.runtime_role_selector != 1 && !(physical_status(target, 0x3E) & 1);
    case 7: return true;
    case 8: return !(physical_status(target, 0x39) & 2) && !(flags & 0x22000);
    case 9: return resources.action_status_kind(target) == 14 && target.runtime_role_selector != 1;
    case 10: return !(target.runtime_role_selector & 3);
    case 11: return !(physical_status(target, 0x3E) & 4);
    default: return false;
    }
}

auto Battle::original_42c88f_bytes(const OriginalEvents& resources, const OriginalEvents::Unit& unit) const -> std::array<uint8_t, 3> { return terrain_effect(resources, unit, { unit.x, unit.y }).bytes; }

auto Battle::turn() const -> unsigned { return turn_; }

auto Battle::hostile_side() const -> bool { return hostile_side_; }

auto Battle::hostile_actions_active() const -> bool { return hostile_actions_active_; }

auto Battle::moving_unit() const -> int { return moving_unit_; }

auto Battle::moving_actor() const -> int { return moving_unit_; }

auto Battle::set_hostile_resources(const OriginalEvents& resources) -> void { hostile_resources_ = std::make_shared<OriginalEvents>(resources); }

auto Battle::physical_action_preview(const OriginalEvents& resources, const OriginalEvents::Unit& actor, const OriginalEvents::Unit& defender, const OriginalEvents::ActionRecord& action, bool counter, int counter_mode) const -> int
{
    s2ne::PhysicalPreviewInput in{};
    in.attack = actor.attack;
    in.defense = defender.defense;
    in.context_bonus = int(resources.action_proficiency(actor, action));
    in.height_difference = s2ne::terrain_height_difference(cell({ actor.x, actor.y }).elevation, cell({ defender.x, defender.y }).elevation, resources.difficulty == 1);
    in.player_side = !actor.hostile();
    in.difficulty_bonus = ai_config_[0xF3 + std::min(5u, resources.difficulty)];
    int mode = int(attack_mode({ actor.x, actor.y }, { defender.x, defender.y }, defender.direction, resources.difficulty == 1));
    if (!counter && counter_mode) { mode = counter_mode; }
    in.counter = counter;
    in.counter_fixed_damage = counter && (actor.abilities & 0x400);
    in.counter_fixed_value = actor.strikeback;
    in.counter_equipment_30 = counter && resources.equipment_special(actor, 30);
    int modeBonus;
    if (counter)
    {
        if (counter_mode == 3) { modeBonus = -3; }
        else if (counter_mode == 2) { modeBonus = -2; }
        else
        {
            modeBonus = 0;
        }
    }
    else if (mode == 3) { modeBonus = 6; }
    else if (mode == 2) { modeBonus = 3; }
    else
    {
        modeBonus = 0;
    }

    in.mode_bonus = modeBonus;
    in.modifiers.class_matchup = resources.class_matchup(actor, defender);
    in.modifiers.action_coefficient = action.fields[11];
    in.modifiers.combat_value = resources.combat_value(actor);
    in.modifiers.defender_116 = uint8_t(defender.abilities >> 8);
    in.modifiers.defender_3c = physical_status(defender, 0x3C);
    in.modifiers.defender_3f = physical_status(defender, 0x3F);
    in.modifiers.defender_39 = physical_status(defender, 0x39);
    in.modifiers.attacker_3e = physical_status(actor, 0x3E);
    in.modifiers.defender_equipment_13 = resources.equipment_special(defender, 0x13);
    auto attacking_terrain = terrain_effect(resources, actor, { actor.x, actor.y });
    auto defending_terrain = terrain_effect(resources, defender, { defender.x, defender.y });
    in.attacker_terrain_bonus = resources.difficulty == 1 ? 0 : int(int8_t(attacking_terrain.bytes[1]));
    in.defender_terrain_bonus = resources.difficulty == 1 ? 0 : int(int8_t(defending_terrain.bytes[2]));
    in.attacker_3c = physical_status(actor, 0x3C);
    in.attacker_118 = uint8_t(actor.abilities >> 24);
    in.defender_kind = uint8_t(defender.runtime_role_selector);
    uint8_t status3d = defender.action_status_runtime ? (*defender.action_status_runtime)[0x3D] : defender.status3d;
    in.immune = (defender.abilities & 0x01000000u) || (status3d & 8);
    // 409BBA+4C1 -> 407C35 uses damage bytes 0/1, not hit bytes 2/3.
    in.multiplier = s2ne::preview_multiplier(ai_config_[0], ai_config_[1]);
    return s2ne::physical_preview(in);
}

auto Battle::ai_plan_diagnostic() const -> const std::optional<AiPlanDiagnostic>& { return ai_plan_diagnostic_; }

auto Battle::automatic_actions_completed() const -> bool { return automatic_actions_completed_; }

auto Battle::begin_npc_actions(std::vector<OriginalEvents::Unit>&) -> void
{
    if (hostile_side_ || hostile_actions_active_)
    {
        return;
    }
    begin_controller_actions(false);
    npc_controller_ = true;
    selected_ = -1;
    state_ = State::idle;
    path_.clear();
    target_ = -1;
}

auto Battle::tick_npc_actions(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds) -> bool { return tick_hostile_actions(units, milliseconds); }

auto Battle::begin_automatic_actions(std::vector<OriginalEvents::Unit>& units) -> void
{
    if (hostile_side_ || hostile_actions_active_)
    {
        return;
    }
    begin_controller_actions(false);
    selected_ = -1;
    state_ = State::idle;
    path_.clear();
    target_ = -1;
}

auto Battle::tick_automatic_actions(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds) -> bool
{
    if (controller_hostile_)
    {
        return automatic_actions_completed_;
    }
    return tick_hostile_actions(units, milliseconds);
}

auto Battle::begin_hostile_actions(std::vector<OriginalEvents::Unit>& units) -> void
{
    if (!hostile_side_)
    {
        begin_hostile_side(units);
    }
    begin_controller_actions(true);
}

auto Battle::begin_controller_actions(bool hostile) -> void
{
    controller_hostile_ = hostile;
    npc_controller_ = false;
    automatic_actions_completed_ = false;
    hostile_actions_active_ = true;
    hostile_cursor_ = 0;
    hostile_actor_ = -1;
    hostile_path_.clear();
    hostile_path_cursor_ = 0;
    hostile_wait_ = 0;
}

auto Battle::idle_pose(OriginalEvents::Unit& unit) -> void
{
    unit.animation_action = 0;
    unit.movement_frame = 0;
    unit.movement_pose = false;
}

auto Battle::tick_hostile_actions(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds) -> bool
{
    if (ai_skill_failed_)
    {
        // 40646D 评分失败后必须清掉候选并重新规划；仅返回会永久锁住电脑行动。
        fail_ai_skill();
        return false;
    }
    if (!hostile_actions_active_)
    {
        return true;
    }
    if (teleport_pending_) { return false; }
    if (animating_movement())
    {
        tick_movement(units, milliseconds);
        return false;
    }
    if (combat_feedback_active())
    {
        tick_combat_feedback(units, milliseconds);
        return false;
    }
    if (ai_skill_waiting_) { return false; }
    if (hostile_wait_)
    {
        hostile_wait_ -= std::min(hostile_wait_, milliseconds);
        return false;
    }
    if (hostile_actor_ >= 0)
    {
        auto& actor = units.at(hostile_actor_);
        if (ai_skill_replan_)
        {
            ai_skill_replan_ = false;
            plan_hostile(units, size_t(hostile_actor_));
            return false;
        }
        if (actor.present(turn_) && hostile_path_cursor_ < hostile_path_.size())
        {
            // 452AD5的class2走452772，只在起终点演出，路线仍决定可达与AP。
            if (hostile_resources_ && hostile_resources_->movement_class(actor) == 2)
            {
                const auto saved = actor;
                while (hostile_path_cursor_ < hostile_path_.size())
                {
                    if (!advance_hostile(units, size_t(hostile_actor_), hostile_path_[hostile_path_cursor_++])) { break; }
                }
                const Point destination{ actor.x, actor.y };
                movement_poses_.clear();
                moving_unit_ = -1;
                actor.direction = saved.direction;
                actor.movement_frame = saved.movement_frame;
                actor.movement_pose = false;
                if (teleport_presenter_ && destination != Point{ saved.x, saved.y })
                {
                    actor.x = saved.x;
                    actor.y = saved.y;
                    teleport_pending_ = teleport_movement_ = true;
                    teleport_destination_ = destination;
                    if (!teleport_presenter_(unsigned(hostile_actor_), destination))
                    {
                        rollback_teleport();
                        actor = saved;
                        report_error(make_error("452772 original presentation unavailable"));
                    }
                }
                return false;
            }
            Point next = hostile_path_[hostile_path_cursor_++];
            if (!advance_hostile(units, size_t(hostile_actor_), next))
            {
                hostile_path_cursor_ = hostile_path_.size();
                if (ai_skill_)
                {
                    report_error(make_error("AI selected spell movement failed; cast remains locked"));
                    fail_ai_skill();
                }
            }
            return false;
        }
        if (actor.present(turn_) && hostile_retreat_)
        {
            if (actor.retreat_role14 != 5 && !(actor.action_status_runtime && ((*actor.action_status_runtime)[0x38] & 2)))
            {
                int kind = actor.appearance && actor.appearance <= ai_kind_classes_.size() ? ai_kind_classes_[actor.appearance - 1] : 0;
                int value = hostile_resources_->combat_value(actor);
                if (s2ne::retreat_40CFE3(0, kind, int16_t(actor.hp), int16_t(actor.max_hp), value, { hostile_resources_->combat_value(actor, 5), hostile_resources_->combat_value(actor, 6), hostile_resources_->combat_value(actor, 7), hostile_resources_->combat_value(actor, 8) }))
                {
                    actor.retreat_role14 = 5;
                }
            }
        }
        if (actor.present(turn_) && !hostile_retreat_ && !hostile_special_)
        {
            if (ai_skill_) { return false; }
            int victim = hostile_target_ >= 0 && can_attack(units, size_t(hostile_actor_), size_t(hostile_target_), hostile_attack_slot_) ? hostile_target_ : hostile_contact_target(units, size_t(hostile_actor_));
            if (victim >= 0 && strike(units, size_t(hostile_actor_), size_t(victim), hostile_attack_slot_))
            {
                hostile_wait_ = 160;
                // 40AA7A returns to action selection while AP permits.
                // Retain the actor until visible strike/feedback has drained.
                hostile_path_.clear();
                hostile_path_cursor_ = 0;
                hostile_target_ = -1;
                hostile_attack_slot_ = -1;
                ai_skill_.reset();
                return false;
            }
        }
        if (original_rules_ && actor.action_points >= config_[8])
        {
            actor.action_points = int(int16_t(actor.action_points - config_[8]));
            actor.carry_points = std::min(actor.carry_points, actor.action_points);
            recover_rest(units, size_t(hostile_actor_));
        }
        idle_pose(actor);
        actor.action_state = 2;
        actor.status34 |= 0x80;
        hostile_actor_ = -1;
        selected_ = -1;
        return false;
    }
    while (hostile_cursor_ < units.size())
    {
        size_t index = hostile_cursor_++;
        bool eligible;
        if (controller_hostile_) { eligible = units[index].hostile(); }
        else if (npc_controller_) { eligible = !units[index].hostile() && units[index].behavior == 1; }
        else
        {
            eligible = units[index].controllable();
        }

        // 原439265／43927F只调度33h的80h已登场单位，剧情待入场NPC不能提前行动。
        if (!eligible || !units[index].present(turn_) || units[index].action_state != 0
            || (original_rules_ && (!(units[index].status_flags & 0x80) || (units[index].status_flags & 0x20))))
        {
            continue;
        }
        hostile_actor_ = int(index);
        selected_ = int(index);
        auto& actor = units[index];
        actor.action_state = 1;
        actor.animation_action = 1;
        hostile_path_.clear();
        hostile_path_cursor_ = 0;
        hostile_target_ = -1;
        hostile_attack_slot_ = -1;
        hostile_retreat_ = false;
        hostile_special_ = false;
        ai_skill_.reset();
        plan_hostile(units, index);
        return false;
    }
    if (controller_hostile_)
    {
        settle_teleport_pads(units, true);
        if (teleport_pending_)
        {
            return false;
        }
        reset_side(units, true);
        tick_action_statuses(units);
        ++turn_;
    }
    else
    {
        automatic_actions_completed_ = true;
    }
    hostile_side_ = false;
    hostile_actions_active_ = false;
    selected_ = -1;
    state_ = State::idle;
    path_.clear();
    target_ = -1;
    message_ = "Turn " + std::to_string(turn_) + (controller_hostile_ ? "; enemy actions completed" : "; automatic player actions completed");
    return true;
}

auto Battle::tick_hostile(const OriginalEvents& resources, std::vector<OriginalEvents::Unit>& units, unsigned milliseconds) -> bool
{
    if (!hostile_actions_active_)
    {
        set_hostile_resources(resources);
        begin_hostile_actions(units);
    }
    return tick_hostile_actions(units, milliseconds);
}

auto Battle::hostile_contact_target(const std::vector<OriginalEvents::Unit>& units, size_t actor) const -> int
{
    if (!hostile_resources_)
    {
        return -1;
    }
    std::vector<OriginalEvents::Unit> own, opponents;
    std::vector<size_t> tokens;
    for (size_t i = 0; i < units.size(); ++i)
    {
        if (units[i].present(turn_))
        {
            if (units[i].hostile() == units[actor].hostile())
            {
                own.push_back(units[i]);
            }
            else
            {
                opponents.push_back(units[i]);
                tokens.push_back(i);
            }
        }
    }
    for (const auto& action : hostile_resources_->action_records(units[actor]))
    {
        if (executable_attack_slot(action) < 0)
        {
            continue;
        }
        for (const auto& target : action_targets(*hostile_resources_, units[actor], action, opponents, own, 0))
        {
            if (can_attack(units, actor, tokens[target.unit_index]))
            {
                return int(tokens[target.unit_index]);
            }
        }
    }
    return -1;
}

auto Battle::advance_hostile(std::vector<OriginalEvents::Unit>& units, size_t index, Point to) -> bool
{
    auto& actor = units[index];
    int cost = step_cost(actor, { actor.x, actor.y }, to);
    if (cost < 0 || !player_route_passable(units, actor, to) || (original_rules_ && cost > actor.action_points - config_[0]))
    {
        return false;
    }
    auto saved = actor;
    unsigned movement_class = hostile_resources_ ? hostile_resources_->movement_class(actor) : 0;
    if (original_rules_)
    {
        actor.action_points -= cost;
        actor.max_action_points = actor.action_points;
        actor.carry_points = std::min(actor.carry_points, actor.action_points);
    }
    actor.direction = facing({ actor.x, actor.y }, to, actor.direction);
    actor.x = to.x;
    actor.y = to.y;
    unsigned int movementFrame = 0;
    if (saved.direction == actor.direction)
    {
        movementFrame = saved.movement_frame;
        if (movement_class != 4 && movement_class != 5) { ++movementFrame; }
    }

    actor.movement_frame = movementFrame;
    actor.action_state = 3;
    actor.status34 |= 1;
    actor.movement_pose = false;
    auto pose = actor;
    pose.movement_pose = true;
    if (movement_class >= 3 && movement_class <= 5)
    {
        pose.movement_visual_class = movement_class;
        pose.movement_camera_pose = true;
    }
    movement_poses_ = { pose };
    moving_unit_ = int(index);
    movement_presented_ = false;
    movement_elapsed_ = 0;
    movement_interval_ = movement_class >= 3 && movement_class <= 5 ? 120 : 80;
    return true;
}

auto Battle::plan_hostile(const std::vector<OriginalEvents::Unit>& units, size_t index) -> void
{
    if (!hostile_resources_)
    {
        return;
    }
    ai_candidate_diagnostics.clear();
    ai_unscored_skills.clear();
    const auto& actor = units[index];
    if (actor.action_points < attack_cost(actor) && (!ai_skills_ || actor.action_points < ai_skills_->action_cost(actor)))
    {
        return;
    }
    Position origin{ actor.x, actor.y };
    auto occupied_unit = [&](Position p)
    {
        return occupant(units, { p.x, p.y }) >= 0;
    };
    auto routes = build_ordered_routes(origin, [&](Position from, Position to) -> std::optional<int>
        {
            int cost = step_cost(actor, { from.x, from.y }, { to.x, to.y });
            if (cost < 0 || !player_route_passable(units, actor, { to.x, to.y }))
            {
                return std::nullopt;
            }
            return cost;
        },
        [&](Position p)
        {
            if (occupied_unit(p)) { return 0; }
            return terrain_effect(*hostile_resources_, actor, { p.x, p.y }).result == 1 ? 1 : 0;
        },
        [&]
        {
            return int(int16_t(actor.action_points - config_[0]));
        },
        [&](int remaining)
        {
            return movement_category(remaining, 1, config_[7], attack_cost(actor));
        });
    std::vector<OriginalEvents::Unit> observers;
    for (const auto& u : units)
    {
        if (u.present(turn_) && u.hostile() == actor.hostile())
        {
            observers.push_back(u);
        }
    }
    std::vector<MovementTarget> targets;
    for (size_t i = 0; i < units.size(); ++i)
    {
        const auto& u = units[i];
        if (u.hostile() == actor.hostile() || !u.present(turn_))
        {
            continue;
        }
        int kind = u.appearance && u.appearance <= ai_kind_classes_.size() ? ai_kind_classes_[u.appearance - 1] : 0;
        targets.push_back({ i, { u.x, u.y }, u.appearance != 0, u.status_flags, kind, hostile_resources_->combat_value(u), !target_visible(u, observers, 0) });
    }
    auto lookup = [&](Position p) -> std::optional<int>
    {
        for (const auto& t : targets)
        {
            if (t.position == p && !t.same_side && (t.status33 & 0x80) && (!(t.status33 & 0x20) || (t.status33 & 2)))
            {
                return t.combat_value;
            }
        }
        return std::nullopt;
    };
    // Ordinary action definitions are scanned at origin, then in the
    // original ordered route table; no distance-sorted attack substitute.
    // 40D2ED tries mode1 before 40B0BA; a failed exact route falls
    // through to that routine's attack scan and later 40AED5(mode0).
    if (actor.ai_role10 == 6 && !(actor.action_status_runtime && ((*actor.action_status_runtime)[0x38] & 2)))
    {
        std::optional<Position> exact_goal;
        for (unsigned code : controller_hostile_ ? std::array<unsigned, 2>{ 3, 5 } : std::array<unsigned, 2>{ 4, 6 })
        {
            int nearest = -1;
            for (unsigned y = 0; y < height_ * 2; ++y)
            {
                for (unsigned x = 0; x < width_; ++x)
                {
                    if (object_category({ int(x), int(y) }) != code)
                    {
                        continue;
                    }
                    Position p{ int(x), int(y) };
                    int distance = s2ne::original_distance(origin, p);
                    if (nearest < 0 || distance < nearest)
                    {
                        nearest = distance;
                        exact_goal = p;
                    }
                }
            }
            if (exact_goal)
            {
                break;
            }
        }
        if (exact_goal)
        {
            for (const auto& record : routes.candidates())
            {
                if (record.marker && record.position.x == exact_goal->x && record.position.y == exact_goal->y)
                {
                    hostile_special_ = true;
                    ai_plan_diagnostic_ = AiPlanDiagnostic{ actor.id, actor.behavior, 0, int(Default40AED5::special_40D351), false, false, true };
                    ai_plan_diagnostic_->destination_record = record.index + 1;
                    for (int node = record.index; node > 0; node = routes.records.at(node).parent)
                    {
                        auto p = routes.records.at(node).route.position;
                        hostile_path_.push_back({ p.x, p.y });
                    }
                    std::reverse(hostile_path_.begin(), hostile_path_.end());
                    return;
                }
            }
        }
    }
    int reach = maximum_route_depth(routes), own_value = hostile_resources_->combat_value(actor);
    bool nearby = nearby_threat(origin, reach, own_value, lookup), ranged = false;
    int kind = actor.appearance && actor.appearance <= ai_kind_classes_.size() ? ai_kind_classes_[actor.appearance - 1] : 0;
    if (!nearby)
    {
        if (auto target = original_target(targets, origin, 3, s2ne::multiply(own_value, 2)))
        {
            const auto& u = units[target->token];
            ranged = target_threat(s2ne::original_distance(origin, target->position), u.base_action_points, u.carry_points, config_[0], config_[6], config_[7], config_[1], hostile_resources_->maximum_action_range(u), reach, false);
        }
    }
    int mode = movement_mode(nearby, ranged);
    Selection attack_selection;
    std::optional<AiSkill> skillSelection;
    if (ai_skills_ && !(physical_status(actor, 0x38) & 2))
    {
        auto scanSkills = [&](Position position, int record)
        {
            auto caster = actor;
            caster.x = position.x;
            caster.y = position.y;
            if (record)
            {
                caster.action_points = routes.records.at(size_t(record - 1)).route.remaining;
            }
            for (bool second : { false, true })
            {
                OriginalSkills::Context context{};
                context.second = second;
                context.computer = true;
                context.army_da_nonzero = hostile_resources_->army_modes && (*hostile_resources_->army_modes)[controller_hostile_ ? 1 : 0] != 0;
                context.suppress_first = hostile_resources_->flags[8] != 0;
                context.suppress_second = hostile_resources_->flags[9] != 0;
                if (!ai_skills_->ai_group_available(caster, context)) { continue; }
                auto probe = *ai_skills_;
                probe.open(caster, context);
                for (const auto& group : probe.groups())
                {
                    for (const auto& skill : group.records)
                    {
                        if (!probe.select_record(skill.id) || probe.resource_failure(caster, skill) != OriginalSkills::Failure::none) { continue; }
                        if (probe.ai_original_unselected()) { continue; }
                        // 原406695恢复族；合作方沿464679/46AC80复核，不借恢复评分放行其它族。
                        const bool damageFamily = (probe.damage_supported() || probe.drain_supported() || probe.fissure29_supported()) && ai_damage_;
                        const bool statusFamily = probe.ai_status_supported();
                        const bool summonFamily = probe.summon_supported();
                        if (summonFamily && !probe.ai_summon_available(caster, *hostile_resources_)) { continue; }
                        if (!(probe.healing1_supported() || probe.area_healing_supported() || skill.id == 88 || damageFamily || statusFamily || summonFamily))
                        {
                            if (std::none_of(ai_unscored_skills.begin(), ai_unscored_skills.end(), [&](const auto& item)
                                    {
                                        return item.skill == skill.id;
                                    }))
                            {
                                ai_unscored_skills.push_back({ actor.id, skill.id, skill.effect(), skill.raw[18] });
                            }
                            continue;
                        }
                        auto proposedUnits = units;
                        proposedUnits[index] = caster;
                        const OriginalEvents::Unit* partner = nullptr;
                        if (skill.raw[32] & 7)
                        {
                            auto p = probe.cooperative_partner(caster, proposedUnits, turn_, [&](int x, int y)
                                {
                                    return cell({ x, y }).elevation;
                                });
                            if (!p || probe.resource_failure(proposedUnits[*p], skill) != OriginalSkills::Failure::none) { continue; }
                            partner = &proposedUnits[*p];
                        }
                        probe.set_area_camera(ai_skill_camera_);
                        probe.set_area_valid([&](int x, int y)
                            {
                                return skill_area_cell({ x, y });
                            });
                        probe.set_area_elevation([&](int x, int y)
                            {
                                return cell({ x, y }).elevation;
                            });
                        for (auto [cx, cy] : probe.ai_centers(caster, [&](int x, int y)
                                 {
                                     return skill_area_cell({ x, y });
                                 }))
                        {
                            const bool directional = damageFamily && (skill.raw[18] == 5 || skill.raw[18] == 8);
                            for (unsigned direction = 0; direction < (directional ? 4u : 1u); ++direction)
                            {
                                probe.set_ai_direction(directional ? std::optional<unsigned>(direction) : std::nullopt);
                                auto centerUnit = caster;
                                centerUnit.x = cx;
                                centerUnit.y = cy;
                                const auto affected = probe.collect_targets(caster, centerUnit, proposedUnits, turn_, [&](int x, int y)
                                    {
                                        return skill_area_cell({ x, y });
                                    });
                                int value = 0;
                                int kills = 0, priorityKills = 0;
                                int statusRank = 0;
                                for (auto target : affected)
                                {
                                    if (statusFamily)
                                    {
                                        auto score = probe.ai_status_value(caster, proposedUnits[target], *hostile_resources_);
                                        if (!statusRank) { statusRank = score.first; }
                                        value += score.second;
                                        continue;
                                    }
                                    if (damageFamily)
                                    {
                                        const auto& victim = proposedUnits[target];
                                        OriginalSkill94Transaction::Context preview;
                                        preview.partner = partner;
                                        auto actorTerrain = original_42c88f_bytes(*hostile_resources_, caster);
                                        auto targetTerrain = original_42c88f_bytes(*hostile_resources_, victim);
                                        preview.hit = { caster.learned_proficiency[skill.group], victim.learned_proficiency[skill.group], caster.fencing, victim.fencing, victim.tactics, int(int8_t(actorTerrain[1])), int(int8_t(targetTerrain[2])), caster.st, victim.st };
                                        preview.elevation = s2ne::terrain_height_difference(cell({ caster.x, caster.y }).elevation, cell({ victim.x, victim.y }).elevation, hostile_resources_->difficulty == 1);
                                        preview.damage.difficulty = hostile_resources_->difficulty;
                                        preview.damage.friendly_army = !caster.hostile();
                                        preview.damage.attack_mode = attack_mode({ caster.x, caster.y }, { victim.x, victim.y }, victim.direction, hostile_resources_->difficulty == 1);
                                        preview.damage.target_status39 = physical_status(victim, 0x39);
                                        preview.damage.target_status3c = physical_status(victim, 0x3C);
                                        preview.damage.target_status3d = physical_status(victim, 0x3D);
                                        preview.damage.target_status3f = physical_status(victim, 0x3F);
                                        preview.damage.target_special13 = hostile_resources_->equipment_special(victim, 13);
                                        auto masks = hostile_resources_->skill_class_masks(victim);
                                        preview.damage.profession_resistance = masks[0];
                                        preview.damage.profession_weakness = masks[1];
                                        auto packet = ai_damage_->preview(skill, caster, victim, *hostile_resources_, preview);
                                        if (!packet)
                                        {
                                            report_error(packet.error());
                                            fail_ai_skill();
                                            return;
                                        }
                                        const int chance = packet.value().chance, amount = packet.value().damage;
                                        const bool priority = victim.id == 1 || victim.id == 11 || (victim.runtime_role_selector & 3);
                                        int weighted = chance * amount;
                                        if (priority) { weighted *= uint8_t(ai_config_[0x100 + std::min(5u, hostile_resources_->difficulty)]); }
                                        if (victim.hostile() == caster.hostile())
                                        {
                                            if (victim.hp < amount || (victim.max_hp > 0 && (victim.hp - amount) * 100 / victim.max_hp < 20))
                                            {
                                                value = 0;
                                                kills = priorityKills = 0;
                                                break;
                                            }
                                            value -= weighted * (100 + amount * 100 / victim.hp) / 100;
                                        }
                                        else
                                        {
                                            if (skill.effect() == 41 && (amount < victim.hp || chance <= 50)) { weighted = 0; }
                                            if (amount >= victim.hp && chance > 50)
                                            {
                                                ++kills;
                                                if (priority) { ++priorityKills; }
                                                weighted *= uint8_t(ai_config_[0x105 + std::min(5u, hostile_resources_->difficulty)]);
                                            }
                                            value += weighted;
                                            if (skill.effect() == 42) { value += chance * std::min(amount, caster.max_hp - caster.hp); }
                                        }
                                        continue;
                                    }
                                    auto score = probe.ai_healing_value(caster, proposedUnits[target], *hostile_resources_, unsigned(uint8_t(ai_config_[0x100 + std::min(5u, hostile_resources_->difficulty)])), partner);
                                    if (!score)
                                    {
                                        report_error(score.error());
                                        ai_skill_failed_ = true;
                                        return;
                                    }
                                    value += score.value();
                                }
                                s2ne::Score candidate{ 3, value, 0 };
                                if (statusFamily) { candidate = { statusRank, value, 0 }; }
                                // 原40646D只在敌方mode2设置rank；召唤mode0虽通过464679，原评分不会选中。
                                if (summonFamily)
                                {
                                    candidate = {};
                                    value = 0;
                                }
                                if (damageFamily) { candidate = { priorityKills ? 1 : 2, value, kills }; }
                                const int campaignCount = hostile_resources_->ending_statistics ? (*hostile_resources_->ending_statistics)[0] : 0;
                                const int threshold = uint8_t(ai_config_[0x10B]) * 100 + campaignCount * uint8_t(ai_config_[0x10C]);
                                if (value > 0 && (!skillSelection || s2ne::improves_skill(candidate, skillSelection->score, threshold)))
                                {
                                    size_t target = index;
                                    if (!affected.empty()) { target = affected.front(); }
                                    skillSelection = AiSkill{ skill.id, target, second, candidate, record, cx, cy };
                                    if (directional) { skillSelection->direction = direction; }
                                }
                            }
                        }
                    }
                }
            }
        };
        scanSkills(origin, 0);
        if (ai_skill_failed_) { return; }
        static constexpr std::array<int, 6> thresholds{ 100, 5, 30, 50, 80, 100 };
        bool moveSkills = percent() <= thresholds[std::min(5u, hostile_resources_->difficulty)];
        if (moveSkills && !(actor.status34 & 1))
        {
            for (const auto& route : routes.candidates())
            {
                if (route.index && route.marker && route.category >= 2)
                {
                    scanSkills(route.position, route.index + 1);
                    if (ai_skill_failed_) { return; }
                }
            }
        }
        if (skillSelection && !s2ne::skill_route_allowed(skillSelection->score, mode)) { skillSelection.reset(); }
        if (skillSelection) { attack_selection.score = skillSelection->score; }
    }
    auto scan = [&](const OriginalEvents::ActionRecord& action, Position position, int record)
    {
        auto moved_actor = actor;
        moved_actor.x = position.x;
        moved_actor.y = position.y;
        // 40B0BA installs candidate runtime23/27 before visibility scans.
        auto moved_observers = observers;
        for (size_t observer = 0; observer < moved_observers.size(); ++observer)
        {
            if (moved_observers[observer].id == actor.id && moved_observers[observer].x == actor.x && moved_observers[observer].y == actor.y)
            {
                moved_observers[observer] = moved_actor;
                break;
            }
        }
        std::vector<OriginalEvents::Unit> opponents;
        std::vector<size_t> tokens;
        for (const auto& t : targets)
        {
            opponents.push_back(units[t.token]);
            tokens.push_back(t.token);
        }
        std::vector<Candidate> candidates;
        for (const auto& target : action_targets(*hostile_resources_, moved_actor, action, opponents, moved_observers, 0))
        {
            const auto& defender = opponents[target.unit_index];
            int previewMode = int(attack_mode({ position.x, position.y }, { defender.x, defender.y }, defender.direction, hostile_resources_->difficulty == 1));
            if (defender.abilities & 0x2000) { previewMode = 1; }
            int damage = physical_action_preview(*hostile_resources_, moved_actor, defender, action, false, previewMode);
            s2ne::HitChanceInput hit{};
            hit.preview = true;
            hit.attacker_tactics = actor.tactics;
            hit.defender_evasion = defender.strikeback;
            hit.height_difference = s2ne::terrain_height_difference(cell({ position.x, position.y }).elevation, cell({ defender.x, defender.y }).elevation, hostile_resources_->difficulty == 1);
            hit.attack_mode = previewMode;
            hit.context_bonus = int(hostile_resources_->action_proficiency(actor, action));
            hit.random_multiplier = s2ne::preview_multiplier(ai_config_[2], ai_config_[3]);
            hit.player_side = !actor.hostile();
            hit.difficulty_bonus = ai_config_[0xF3 + std::min(5u, hostile_resources_->difficulty)];
            hit.modifiers.action_type = 21;
            hit.modifiers.action_bonus = hostile_resources_->action_accuracy_bonus(actor, action);
            hit.modifiers.defender_equipment_penalty = hostile_resources_->equipment_evasion(defender);
            hit.modifiers.defender_115 = uint8_t(defender.abilities);
            hit.modifiers.defender_3c = defender.status3c;
            hit.modifiers.defender_equipment_16 = hostile_resources_->equipment_special(defender, 0x16);
            hit.defender_39 = defender.status39.value_or(0);
            hit.defender_3d = defender.status3d;
            hit.defender_status3c_dword = defender.status3c;
            if (defender.action_status_runtime)
            {
                hit.defender_39 = (*defender.action_status_runtime)[0x39];
                hit.defender_3d = (*defender.action_status_runtime)[0x3D];
                hit.defender_status3c_dword = 0;
                for (unsigned i = 0; i < 4; ++i)
                {
                    hit.defender_status3c_dword |= uint32_t((*defender.action_status_runtime)[0x3C + i]) << (8 * i);
                }
            }
            hit.preview_immune = (defender.abilities & 0x01000000u) != 0;
            int chance = s2ne::hit_chance(hit);
            MeleePreview preview{};
            preview.attack_value = physical_preview_value(chance, damage);
            preview.attack_damage = damage;
            preview.attack_chance = chance;
            preview.target_hp = defender.hp;
            preview.attacker_hp = actor.hp;
            preview.target_kind = int(defender.id);
            preview.target_flags2f = defender.runtime_role_selector;
            preview.attacker_status38_02 = (physical_status(actor, 0x38) & 2) != 0;
            int counterAction = -1;
            auto counters = counter_actions(*hostile_resources_, defender, { position.x, position.y },
                std::span<const OriginalEvents::Unit>(&moved_actor, 1), opponents, 0,
                physical_status(defender, 0x39), physical_status_dword(defender));
            if (!counters.empty())
            {
                // 原409A2D每次候选预估也抽反击池，不能取最大值或保留RNG不变。
                unsigned selected = counters[size_t(roll(int(counters.size())))];
                counterAction = int(selected);
                for (const auto& counter : hostile_resources_->action_records(defender))
                {
                    if (counter.index != selected) { continue; }
                    int mode = previewMode;
                    auto reverse = hit;
                    reverse.counter_preview = true;
                    reverse.attacker_tactics = defender.tactics;
                    reverse.defender_evasion = actor.strikeback;
                    reverse.height_difference = -hit.height_difference;
                    reverse.attack_mode = mode;
                    reverse.context_bonus = int(hostile_resources_->action_proficiency(defender, counter));
                    reverse.player_side = !defender.hostile();
                    reverse.counter_accuracy_penalty = (physical_status(defender, 0x3D) & 0x10) != 0;
                    reverse.modifiers.action_bonus = hostile_resources_->action_accuracy_bonus(defender, counter);
                    reverse.modifiers.defender_equipment_penalty = hostile_resources_->equipment_evasion(actor);
                    reverse.modifiers.defender_115 = uint8_t(actor.abilities);
                    reverse.modifiers.defender_3c = physical_status(actor, 0x3C);
                    reverse.modifiers.defender_equipment_16 = hostile_resources_->equipment_special(actor, 16);
                    reverse.defender_39 = physical_status(actor, 0x39);
                    reverse.defender_3d = physical_status(actor, 0x3D);
                    reverse.defender_status3c_dword = physical_status_dword(actor);
                    reverse.preview_immune = (actor.abilities & 0x01000000u) != 0;
                    preview.counter_chance = s2ne::hit_chance(reverse);
                    preview.counter_damage = physical_action_preview(*hostile_resources_, defender, moved_actor, counter, true, mode);
                    if (preview_action_weight(*hostile_resources_, moved_actor, defender, action))
                    {
                        preview.attack_value = s2ne::physical_preview_value(chance + 10, damage);
                    }
                    int weight = preview_action_weight(*hostile_resources_, defender, moved_actor, counter) ? 10 : 0;
                    preview.counter_value = s2ne::physical_preview_value(preview.counter_chance + weight, preview.counter_damage);
                    break;
                }
            }
            else if (preview_action_weight(*hostile_resources_, moved_actor, defender, action))
            {
                preview.attack_value = s2ne::physical_preview_value(chance + 10, damage);
            }
            unsigned difficulty = std::min(5u, hostile_resources_->difficulty);
            auto score = s2ne::score_melee(preview, { uint8_t(ai_config_[0x100 + difficulty]), uint8_t(ai_config_[0x105 + difficulty]) });
            ai_candidate_diagnostics.push_back({ actor.id, defender.id, action.index, chance, damage, s2ne::physical_preview_value(chance, damage), counterAction, preview.counter_chance, preview.counter_damage, preview.counter_value, preview.attack_value, score ? score->value : 0,
                preview.counter_damage > actor.hp && preview.counter_chance > 50 && !preview.attacker_status38_02 });
            candidates.push_back({ tokens[target.unit_index], record, preview, int(action.index) });
        }
        unsigned difficulty = std::min(5u, hostile_resources_->difficulty);
        const int campaignCount = hostile_resources_->ending_statistics ? (*hostile_resources_->ending_statistics)[0] : 0;
        if (scan_melee_position(candidates, { uint8_t(ai_config_[0x100 + difficulty]), uint8_t(ai_config_[0x105 + difficulty]) }, attack_selection,
                uint8_t(ai_config_[0x10B]) * 100 + campaignCount * uint8_t(ai_config_[0x10C])))
        {
            return true;
        }
        return false;
    };
    // 原40B5B3先遍历动作，每个动作先原地再遍历路线；rank1立即结束整个扫描。
    // 若先遍历位置，后一个动作的原地击杀会错误抢在前一个动作的移动击杀之前。
    for (const auto& action : hostile_resources_->action_records(actor))
    {
        if (executable_attack_slot(action) < 0) { continue; }
        if (s2ne::scan_action_positions(routes.candidates(), (actor.status34 & 1) != 0, [&](Position position, int record)
                {
                    return scan(action, position, record);
                },
                origin))
        {
            break;
        }
    }
    if (attack_selection.candidate)
    {
        hostile_target_ = int(attack_selection.candidate->target);
        for (const auto& action : hostile_resources_->action_records(actor))
        {
            if (int(action.index) == attack_selection.candidate->action_index)
            {
                hostile_attack_slot_ = executable_attack_slot(action);
            }
        }
        int node = attack_selection.candidate->destination_record - 1;
        for (; node > 0; node = routes.records.at(node).parent)
        {
            auto p = routes.records.at(node).route.position;
            hostile_path_.push_back({ p.x, p.y });
        }
        std::reverse(hostile_path_.begin(), hostile_path_.end());
        return;
    }
    if (skillSelection)
    {
        ai_skill_ = skillSelection;
        ai_skill_actor_ = index;
        for (int node = skillSelection->destination_record - 1; node > 0; node = routes.records.at(node).parent)
        {
            auto position = routes.records.at(node).route.position;
            hostile_path_.push_back({ position.x, position.y });
        }
        std::reverse(hostile_path_.begin(), hostile_path_.end());
        return;
    }
    // 40AED5 reads the high WORD at runtime+8 (+0Ah), not control+8.
    auto dispatch = dispatch_40AED5(actor.status34, actor.action_status_runtime ? (*actor.action_status_runtime)[0x38] : 0, actor.ai_role10 >= 0 ? actor.ai_role10 : actor.behavior, int(hostile_resources_->difficulty), 0, controller_hostile_ ? 1 : 0);
    ai_plan_diagnostic_ = AiPlanDiagnostic{ actor.id, actor.behavior, kind, int(dispatch), dispatch == Default40AED5::goal_43939D, dispatch == Default40AED5::special_40D351, true };
    ai_plan_diagnostic_->movement_mode = mode;
    ai_plan_diagnostic_->route_count = int(routes.order.size());
    if (dispatch == Default40AED5::goal_43939D)
    {
        // 43939D chooses the nearest target having a usable adjacent tile;
        // 40AFE6 then calls 47736A with reserve=0, not the ordinary controller.
        const MovementTarget* goal = nullptr;
        int nearest = 0;
        for (const auto& candidate : targets)
        {
            if (!candidate.runtime_ad_nonzero || (candidate.status33 & 0x20) || !(candidate.status33 & 0x80))
            {
                continue;
            }
            Point target{ candidate.position.x, candidate.position.y };
            bool approachable = false;
            for (auto p : std::array<Point, 4>{ { { target.x + (target.y & 1), target.y - 1 }, { target.x - 1 + (target.y & 1), target.y - 1 }, { target.x + (target.y & 1), target.y + 1 }, { target.x - 1 + (target.y & 1), target.y + 1 } } })
            {
                if (p != Point{ actor.x, actor.y } && (occupant(units, p) >= 0 || step_cost(actor, target, p) < 0))
                {
                    continue;
                }
                int delta = cell(p).elevation - cell(target).elevation;
                if (cell(p).elevation > 0 && delta >= -2 && delta <= 3)
                {
                    approachable = true;
                    break;
                }
            }
            if (!approachable)
            {
                continue;
            }
            int distance = s2ne::original_distance(origin, candidate.position);
            if (!goal || distance < nearest)
            {
                goal = &candidate;
                nearest = distance;
                if (distance == 1)
                {
                    break;
                }
            }
        }
        if (!goal)
        {
            return;
        }
        auto destination = select_toward(routes.candidates(), goal->position, 0, [&](Position p)
            {
                if (actor.movement_type >= 6 && actor.movement_type <= 10)
                {
                    return false;
                }
                auto tile = cell({ p.x, p.y });
                if (tile.ground <= 4 || ground_attributes_.empty())
                {
                    return false;
                }
                unsigned category = ground_attributes_.at(tile.ground - 1) >> 8;
                return category == 1 || category == 2;
            },
            [](Position)
            {
                return false;
            });
        ai_plan_diagnostic_->unsupported_goal = false;
        ai_plan_diagnostic_->target_id = units[goal->token].id;
        if (!destination)
        {
            return;
        }
        ai_plan_diagnostic_->destination_record = destination->destination_record;
        for (int node = destination->destination_record - 1; node > 0; node = routes.records.at(node).parent)
        {
            auto p = routes.records.at(node).route.position;
            hostile_path_.push_back({ p.x, p.y });
        }
        std::reverse(hostile_path_.begin(), hostile_path_.end());
        return;
    }
    if (dispatch == Default40AED5::special_40D351)
    {
        hostile_special_ = true;
        std::optional<Position> goal;
        for (unsigned code : controller_hostile_ ? std::array<unsigned, 2>{ 3, 5 } : std::array<unsigned, 2>{ 4, 6 })
        {
            int nearest = -1;
            for (unsigned y = 0; y < height_ * 2; ++y)
            {
                for (unsigned x = 0; x < width_; ++x)
                {
                    if (object_category({ int(x), int(y) }) != code)
                    {
                        continue;
                    }
                    Position p{ int(x), int(y) };
                    int distance = s2ne::original_distance(origin, p);
                    if (nearest < 0 || distance < nearest)
                    {
                        nearest = distance;
                        goal = p;
                    }
                }
            }
            if (goal)
            {
                break;
            }
        }
        ai_plan_diagnostic_->unsupported_special = false;
        if (!goal)
        {
            return;
        }
        std::optional<RouteRecord> best;
        int distance = -1;
        for (const auto& r : routes.candidates())
        {
            if (!r.marker)
            {
                continue;
            }
            if (r.position.x == goal->x && r.position.y == goal->y)
            {
                best = r;
                break;
            }
            if (occupied_unit(r.position))
            {
                continue;
            }
            int d = s2ne::original_distance(r.position, *goal);
            if (distance < 0 || d < distance)
            {
                distance = d;
                best = r;
            }
        }
        if (!best)
        {
            return;
        }
        ai_plan_diagnostic_->destination_record = best->index + 1;
        for (int node = best->index; node > 0; node = routes.records.at(node).parent)
        {
            auto p = routes.records.at(node).route.position;
            hostile_path_.push_back({ p.x, p.y });
        }
        std::reverse(hostile_path_.begin(), hostile_path_.end());
        return;
    }
    if (dispatch != Default40AED5::controller_40D171)
    {
        return;
    }
    auto behavior = dispatch_40D171(mode, int16_t(actor.hp), int16_t(actor.max_hp), [&](int m)
        {
            unsigned armyFlags = 0;
            if (hostile_resources_->army_modes) { armyFlags = (*hostile_resources_->army_modes)[controller_hostile_ ? 1 : 0]; }
            const bool urgent = urgent_40CDA1(m, kind, actor.hp, actor.max_hp, actor.mp, actor.max_mp,
                (actor.abilities & 0x80) || (actor.status3c & 0x10), hostile_resources_->difficulty, controller_hostile_ ? 1 : 0, armyFlags);
            return int(urgent);
        },
        [&](int m)
        {
            return retreat_40CFE3(m, kind, int16_t(actor.hp), int16_t(actor.max_hp), own_value,
                       { hostile_resources_->combat_value(actor, 5), hostile_resources_->combat_value(actor, 6), hostile_resources_->combat_value(actor, 7), hostile_resources_->combat_value(actor, 8) }) ?
                1 :
                0;
        });
    auto records = routes.candidates();
    if (behavior == Behavior40D171::retreat)
    {
        if (actor.max_hp <= 0)
        {
            ai_plan_diagnostic_->unsupported_health = true;
            return;
        }
        auto destination = select_retreat(records, actor.hp * 100 / actor.max_hp < 90, int(int8_t(ai_config_[0xF0])), [&](Position p)
            {
                return occupant(units, { p.x, p.y }) >= 0;
            },
            [&](Position p)
            {
                return original_target(targets, p, 0, 0);
            });
        hostile_retreat_ = true;
        ai_plan_diagnostic_->unsupported_health = false;
        if (!destination)
        {
            return;
        }
        ai_plan_diagnostic_->destination_record = destination->destination_record;
        for (int node = destination->destination_record - 1; node > 0; node = routes.records.at(node).parent)
        {
            auto p = routes.records.at(node).route.position;
            hostile_path_.push_back({ p.x, p.y });
        }
        std::reverse(hostile_path_.begin(), hostile_path_.end());
        return;
    }
    auto blocked_terrain = [&](Position p)
    {
        if (actor.movement_type >= 6 && actor.movement_type <= 10)
        {
            return false;
        }
        auto tile = cell({ p.x, p.y });
        if (tile.ground <= 4 || ground_attributes_.empty())
        {
            return false;
        }
        unsigned category = ground_attributes_.at(tile.ground - 1) >> 8;
        return category == 1 || category == 2;
    };
    auto adjacent_opponent = [&](Position p)
    {
        return original_adjacent(p, [&](Position q)
            {
                return lookup(q) ? 1 : 0;
            });
    };
    if (!targets.empty())
    {
        ai_plan_diagnostic_->target_id = units[targets.front().token].id;
        ai_plan_diagnostic_->target_value = targets.front().combat_value;
        ai_plan_diagnostic_->target_hidden = targets.front().same_side;
    }
    auto destination = select_40D25A(records, targets, origin, behavior == Behavior40D171::approach_urgent, config_[5], config_[8], blocked_terrain, adjacent_opponent);
    if (!actor.hostile() && !targets.empty())
    {
        // 获准差异：用合法地形的剩余路程代替直线接近，允许先远离目标绕上高地。
        std::vector<int> remaining(cells_.size(), std::numeric_limits<int>::max());
        using Node = std::pair<int, int>;
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> queue;
        for (const auto& target : targets)
        {
            if (target.same_side) { continue; }
            Point p{ target.position.x, target.position.y };
            if (!valid(p)) { continue; }
            int cellIndex = p.y * int(width_) + p.x;
            remaining[cellIndex] = 0;
            queue.push({ 0, cellIndex });
        }
        while (!queue.empty())
        {
            auto [cost, cellIndex] = queue.top();
            queue.pop();
            if (remaining[cellIndex] != cost) { continue; }
            Point to{ cellIndex % int(width_), cellIndex / int(width_) };
            for (int dy : { -1, 1 })
            {
                for (int dx : { 0, (to.y & 1) ? 1 : -1 })
                {
                    Point from{ to.x + dx, to.y + dy };
                    if (!valid(from)) { continue; }
                    int edge = step_cost(actor, from, to);
                    if (edge < 0) { continue; }
                    int next = from.y * int(width_) + from.x;
                    if (cost + edge < remaining[next])
                    {
                        remaining[next] = cost + edge;
                        queue.push({ cost + edge, next });
                    }
                }
            }
        }
        int best = remaining[origin.y * int(width_) + origin.x];
        const RouteRecord* chosen = nullptr;
        for (const auto& record : records)
        {
            if (!record.marker || occupied_unit(record.position)) { continue; }
            int cost = remaining[record.position.y * int(width_) + record.position.x];
            if (cost < best)
            {
                best = cost;
                chosen = &record;
            }
        }
        if (chosen)
        {
            hostile_path_.clear();
            ai_plan_diagnostic_->destination_record = chosen->index + 1;
            for (int node = chosen->index; node > 0; node = routes.records.at(node).parent)
            {
                auto p = routes.records.at(node).route.position;
                hostile_path_.push_back({ p.x, p.y });
            }
            std::reverse(hostile_path_.begin(), hostile_path_.end());
            return;
        }
    }
    if (!destination)
    {
        return;
    }
    ai_plan_diagnostic_->destination_record = destination->destination_record;
    for (int node = destination->destination_record - 1; node > 0; node = routes.records.at(node).parent)
    {
        auto p = routes.records.at(node).route.position;
        hostile_path_.push_back({ p.x, p.y });
    }
    std::reverse(hostile_path_.begin(), hostile_path_.end());
}

auto Battle::begin_hostile_side(std::vector<OriginalEvents::Unit>& units) -> void
{
    for (auto& unit : units)
    {
        if (unit.controllable() && unit.present(turn_))
        {
            unit.action_state = 2;
        }
    }
    reset_side(units, false);
    hostile_side_ = true;
    selected_ = -1;
    state_ = State::idle;
    path_.clear();
    target_ = -1;
}

auto Battle::tick_action_statuses(std::vector<OriginalEvents::Unit>& units) -> void
{
    for (auto& unit : units)
    {
        if (unit.action_status_runtime)
        {
            auto& bytes = *unit.action_status_runtime;
            bytes[0x33] = uint8_t(unit.status_flags);
            s2ne::tick_status_durations(bytes);
            unit.sp = int(int16_t(unsigned(bytes[0x12A]) | (unsigned(bytes[0x12B]) << 8)));
            unit.max_action_points = int(int16_t(unsigned(bytes[0x12C]) | (unsigned(bytes[0x12D]) << 8)));
            unit.status39 = bytes[0x39];
            unit.status3c = bytes[0x3C];
            unit.attack = int(int16_t(unsigned(bytes[0x134]) | (unsigned(bytes[0x135]) << 8)));
            unit.defense = int(int16_t(unsigned(bytes[0x136]) | (unsigned(bytes[0x137]) << 8)));
        }
    }
}

auto Battle::reset_side(std::vector<OriginalEvents::Unit>& units, bool hostile) -> void
{
    for (auto& u : units)
    {
        if (u.present(turn_) && u.hostile() == hostile && u.action_state == 2)
        {
            int carry = u.base_action_points > config_[5] ? config_[5] : u.action_points;
            if (u.action_points + u.base_action_points >= 110)
            {
                carry = 0;
            }
            u.carry_points = carry;
            u.action_points = u.max_action_points = u.base_action_points + carry;
            u.status34 = 0;
            u.action_state = 0;
        }
    }
}

auto Battle::end_turn(std::vector<OriginalEvents::Unit>& units) -> void
{
    if (!hostile_side_)
    {
        begin_hostile_side(units);
    }
    unsigned responses = 0;
    for (size_t a = 0; a < units.size(); ++a)
    {
        if (!units[a].hostile() || !units[a].present(turn_))
        {
            continue;
        }
        for (size_t b = 0; b < units.size(); ++b)
        {
            if (units[b].present(turn_) && !units[b].hostile() && adjacent({ units[a].x, units[a].y }, { units[b].x, units[b].y }) && cell({ units[a].x, units[a].y }).elevation == cell({ units[b].x, units[b].y }).elevation)
            {
                if (strike(units, a, b))
                {
                    ++responses;
                }
                break;
            }
        }
        units[a].action_state = 2;
    }
    settle_teleport_pads(units, true);
    reset_side(units, true);
    ++turn_;
    hostile_side_ = false;
    selected_ = -1;
    state_ = State::idle;
    path_.clear();
    target_ = -1;
    message_ = "Turn " + std::to_string(turn_) + "; enemy contact attacks " + std::to_string(responses) + "; Enter: finish side";
}

auto Battle::present_attack(const std::vector<OriginalEvents::Unit>& units, size_t index, const OriginalEvents::ActionRecord* chosen) -> void
{
    if (!movement_animation_enabled_ || !hostile_resources_)
    {
        return;
    }
    const auto& actor = units[index];
    if (!actor.appearance || actor.appearance > action_sequences_.size())
    {
        return;
    }
    std::vector<OriginalEvents::Unit> own;
    for (const auto& unit : units)
    {
        if (unit.present(turn_) && unit.hostile() == actor.hostile())
        {
            own.push_back(unit);
        }
    }
    for (const auto& action : hostile_resources_->action_records(actor))
    {
        if (executable_attack_slot(action) < 0 || (chosen && chosen->index != action.index))
        {
            continue;
        }
        bool supported = chosen != nullptr;
        for (size_t target = 0; target < units.size(); ++target)
        {
            if (can_attack(units, index, target))
            {
                if (!action_targets(*hostile_resources_, actor, action, std::span<const OriginalEvents::Unit>(&units[target], 1), own, 0).empty())
                {
                    supported = true;
                    break;
                }
            }
        }
        if (!supported)
        {
            continue;
        }
        unsigned pose_action = uint8_t(action.fields[10]), sequence = pose_action * 4 + (actor.direction & 3);
        if (sequence >= 30)
        {
            return;
        }
        const auto& frames = action_sequences_[actor.appearance - 1][sequence];
        if (frames.empty())
        {
            return;
        }
        movement_poses_.clear();
        for (size_t frame = 0; frame < frames.size(); ++frame)
        {
            auto pose = actor;
            pose.animation_action = pose_action;
            pose.movement_pose = true;
            pose.movement_frame = unsigned(frame);
            movement_poses_.push_back(pose);
        }
        moving_unit_ = int(index);
        movement_presented_ = false;
        movement_elapsed_ = 0;
        movement_interval_ = 120;
        attack_animation_ = true;
        if (effect_audio_)
        {
            int sound = original_action_pose_sound(action.fields);
            if (sound > 0) { effect_audio_(unsigned(sound), 0); }
        }
        return;
    }
}

auto Battle::attack(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot) -> bool
{
    if (a >= units.size() || !player_controlled(units[a], a) || !can_attack(units, a, b, slot) || animating_movement())
    {
        return false;
    }
    return strike(units, a, b, slot);
}

auto Battle::physical_feedback(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int damage, bool miss) -> void
{
    reaction_pose_.reset();
    units[b].ending_combat_value = units[b].hp;
    const bool waiting = attack_animation_active() || projectile_active();
    if (!movement_animation_enabled_ || !waiting)
    {
        if (effect_audio_ && physical_sound_ > 0) { effect_audio_(unsigned(physical_sound_), 0); }
        physical_sound_ = 0;
    }
    if (movement_animation_enabled_)
    {
        combat_feedback_.begin(a, b, units[b].hp, damage, miss, attack_animation_active() || projectile_active(), units[b].max_hp);
        if (!miss && damage > 0 && attack_animation_active())
        {
            reaction_pose_ = units[b];
        }
        return;
    }
    int lost = damage < 0 ? -std::min(units[b].max_hp - units[b].hp, -damage) : std::min(units[b].hp, damage);
    units[b].hp -= lost;
    settle_physical(units, a, b, lost);
}

auto Battle::draw_forward_damage(const std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, const OriginalEvents::ActionRecord& action, int attack_style, int* reverse_tactics) -> int
{
    const auto& attacker = units[a];
    const auto& defender = units[b];
    const auto& resources = *hostile_resources_;
    bool simple_rules = resources.difficulty == 1;
    auto attacking_terrain = terrain_effect(resources, attacker, { attacker.x, attacker.y });
    auto defending_terrain = terrain_effect(resources, defender, { defender.x, defender.y });
    int height = terrain_height_difference(cell({ attacker.x, attacker.y }).elevation, cell({ defender.x, defender.y }).elevation, simple_rules);
    if (simple_rules)
    {
        attacking_terrain.bytes[1] = 0;
        defending_terrain.bytes[2] = 0;
    }
    // 408D4C reads [runtime136] SAR16 == current tactics138;
    // 408DD0 subtracts [runtime0BF] SAR16 == evasion0C1.
    HitChanceInput hit{};
    hit.attacker_tactics = attacker.tactics;
    hit.defender_evasion = defender.strikeback;
    hit.height_difference = height;
    hit.attack_mode = attack_style;
    hit.context_bonus = int(resources.action_proficiency(attacker, action));
    hit.attacker_condition = attacker.max_hp ? lost_health_percent(attacker.max_hp, attacker.hp) : 0;
    hit.condition_roll = hit.attacker_condition > 70 ? percent() : 0;
    // 408CB7 adjusts both tactics words before the forward hit draw.
    int defender_condition = defender.max_hp ? lost_health_percent(defender.max_hp, defender.hp) : 0;
    int defender_roll = defender_condition > 70 ? percent() : 0;
    if (reverse_tactics) { *reverse_tactics = adjusted_tactics(defender.tactics, defender_condition, defender_roll); }
    hit.random_multiplier = ai_config_[2] + roll(ai_config_[3] + 3);
    hit.player_side = !attacker.hostile();
    hit.difficulty_bonus = ai_config_[0xF3 + std::min(5u, resources.difficulty)];
    hit.modifiers.action_type = 21;    // action_accuracy_bonus has already applied weapon accuracy.
    hit.modifiers.action_bonus = resources.action_accuracy_bonus(attacker, action);
    hit.modifiers.defender_equipment_penalty = resources.equipment_evasion(defender);
    hit.modifiers.defender_115 = uint8_t(defender.abilities);
    hit.modifiers.defender_3c = defender.status3c;
    hit.modifiers.defender_equipment_16 = resources.equipment_special(defender, 0x16);
    hit.defender_39 = defender.status39.value_or(0);
    hit.defender_3d = defender.status3d;
    hit.defender_status3c_dword = defender.status3c;
    if (defender.action_status_runtime)
    {
        hit.defender_39 = (*defender.action_status_runtime)[0x39];
        hit.defender_3d = (*defender.action_status_runtime)[0x3D];
        hit.defender_status3c_dword = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            hit.defender_status3c_dword |= uint32_t((*defender.action_status_runtime)[0x3C + i]) << (8 * i);
        }
    }
    int hitBonus = 0;
    if (attack_style == 3) { hitBonus = 5; }
    else if (attack_style == 2) { hitBonus = 3; }
    last_physical_diagnostic_ = PhysicalDiagnostic{ a, b, std::clamp(attacker.tactics - defender.strikeback + hitBonus, 1, 100) * 12 / 10,
        hit_chance(hit), hit.modifiers.action_bonus, height, attack_style };
    if (!hit_succeeds(hit, percent())) { return 0; }
    PhysicalDamageInput damage{};
    damage.attack = attacker.attack;
    damage.defense = defender.defense;
    damage.height_difference = height;
    damage.attack_mode = attack_style;
    damage.context_bonus = hit.context_bonus;
    damage.attacker_terrain_bonus = int(int8_t(attacking_terrain.bytes[1]));
    damage.defender_terrain_bonus = int(int8_t(defending_terrain.bytes[2]));
    int chance = percent(), decimal = roll(10);
    damage.divisor = damage_divisor(chance, decimal);
    damage.random_multiplier = ai_config_[0] + roll(ai_config_[1] + 3);
    damage.player_side = !attacker.hostile();
    damage.difficulty_bonus = hit.difficulty_bonus;
    damage.attacker_3c = attacker.status3c;
    damage.attacker_118 = uint8_t(attacker.abilities >> 24);
    damage.defender_kind = uint8_t(defender.kind_attribute);
    damage.modifiers.action_coefficient = action.fields[11];
    damage.modifiers.class_matchup = resources.class_matchup(attacker, defender);
    damage.modifiers.combat_value = resources.combat_value(attacker);
    damage.modifiers.defender_116 = uint8_t(defender.abilities >> 8);
    damage.modifiers.defender_3c = physical_status(defender, 0x3C);
    damage.modifiers.defender_3f = physical_status(defender, 0x3F);
    damage.modifiers.defender_39 = physical_status(defender, 0x39);
    damage.modifiers.attacker_3e = physical_status(attacker, 0x3E);
    damage.modifiers.defender_equipment_13 = resources.equipment_special(defender, 0x13);
    // The minimum-damage draw only occurs after all modifiers.
    damage.minimum_roll = 99;
    int dealt = physical_damage(damage);
    if (dealt < 3)
    {
        damage.minimum_roll = percent();
        dealt = physical_damage(damage);
    }

    return dealt;
}

auto Battle::draw_physical_consecutive(const PhysicalActionContext& context, bool reverse) -> bool
{
    const auto& actor = reverse ? context.target_snapshot : context.actor_snapshot;
    const auto& target = reverse ? context.actor_snapshot : context.target_snapshot;
    const auto& resources = *hostile_resources_;
    s2ne::ConsecutiveInput input{};
    input.has_attack_context = physical_action_preview(resources, actor, target, reverse ? *context.counter : context.action, reverse, context.mode) != 0;
    input.attacker_tactics = actor.tactics;
    input.defender_evasion = target.strikeback;
    input.mode_adjustment = s2ne::consecutive_mode_adjustment(context.mode, reverse);
    input.attacker_equipment_bonus = resources.equipment_accuracy(actor);
    input.defender_equipment_penalty = resources.equipment_evasion(target);
    input.attacker_115 = uint8_t(actor.abilities);
    input.defender_115 = uint8_t(target.abilities);
    input.defender_3c = physical_status(target, 0x3C);
    input.defender_3f = physical_status(target, 0x3F);
    input.defender_equipment_13 = resources.equipment_special(target, 13);
    input.defender_equipment_16 = resources.equipment_special(target, 16);
    return s2ne::consecutive_succeeds(input, percent());
}

auto Battle::draw_physical_instant(const PhysicalActionContext& context) -> bool
{
    const auto& actor = context.actor_snapshot;
    const auto& target = context.target_snapshot;
    const auto& resources = *hostile_resources_;
    if (!physical_action_preview(resources, actor, target, context.action) || target.id == 1 || target.id == 11 || (physical_status(target, 0x3F) & 4) || resources.equipment_special(target, 13)) { return false; }
    int threshold = 0;
    if (resources.action_status_kind(target) == 14 && target.runtime_role_selector == 1 && resources.equipment_special(actor, 62)) { threshold = 50; }
    else if ((actor.abilities & 0x200) && !(target.runtime_role_selector & 3)) { threshold = 5; }
    return percent() < threshold;
}

auto Battle::try_forward_exchange(const OriginalEvents& resources, const std::vector<OriginalEvents::Unit>& units, size_t actor, size_t primary, const OriginalEvents::ActionRecord& action) -> Result<std::vector<PhysicalExchangeStep>>
{
    for (const auto& unit : units)
    {
        if (resources.equipment_special(unit, 28) && (!physical_equipment_ || !physical_equipment_->first_consumable_special(unit, 28)))
        {
            return Result<std::vector<PhysicalExchangeStep>>::failure(make_error("Original resurrection equipment consumption unavailable"));
        }
    }
    auto baseline = random_;
    auto contexts = try_physical_contexts(resources, units, actor, primary, action);
    if (!contexts) { return Result<std::vector<PhysicalExchangeStep>>::failure(contexts.error()); }
    std::vector<PhysicalExchangeStep> steps;
    for (auto& context : contexts.value())
    {
        if (context.actor_snapshot.max_hp <= 0 || context.target_snapshot.max_hp <= 0)
        {
            random_ = baseline;
            return Result<std::vector<PhysicalExchangeStep>>::failure(make_error("Original physical HP context"));
        }
        PhysicalExchangeStep step{ std::move(context) };
        auto& c = step.context;
        if (physical_action_preview(resources, c.actor_snapshot, c.target_snapshot, c.action))
        {
            step.damage = draw_forward_damage(units, c.actor, c.target, c.action, c.mode, &c.counter_tactics);
        }
        step.consecutive = draw_physical_consecutive(c, false);
        if (draw_physical_instant(c))
        {
            step.damage = c.target_snapshot.hp;
            step.consecutive = false;
        }
        if (step.consecutive)
        {
            step.damage += draw_forward_damage(units, c.actor, c.target, c.action, c.mode, &c.counter_tactics);
            if (!step.damage) { step.consecutive = false; }
        }
        steps.push_back(std::move(step));
    }
    return Result<std::vector<PhysicalExchangeStep>>::success(std::move(steps));
}

auto Battle::draw_reverse_damage(const PhysicalActionContext& context) -> int
{
    const auto& actor = context.target_snapshot;
    const auto& target = context.actor_snapshot;
    const auto& action = *context.counter;
    const auto& resources = *hostile_resources_;
    if (!physical_action_preview(resources, actor, target, action, true, context.mode)) { return 0; }
    bool simple = resources.difficulty == 1;
    int height = terrain_height_difference(cell({ actor.x, actor.y }).elevation, cell({ target.x, target.y }).elevation, simple);
    HitChanceInput hit{};
    hit.counter = true;
    hit.attacker_tactics = context.counter_tactics;
    hit.defender_evasion = target.strikeback;
    hit.height_difference = height;
    hit.attack_mode = context.mode;
    hit.context_bonus = int(resources.action_proficiency(actor, action));
    hit.player_side = !actor.hostile();
    hit.difficulty_bonus = ai_config_[0xF3 + std::min(5u, resources.difficulty)];
    hit.counter_accuracy_penalty = (physical_status(actor, 0x3D) & 0x10) != 0;
    hit.modifiers.action_type = 21;
    hit.modifiers.action_bonus = resources.action_accuracy_bonus(actor, action);
    hit.modifiers.defender_equipment_penalty = resources.equipment_evasion(target);
    hit.modifiers.defender_115 = uint8_t(target.abilities);
    hit.modifiers.defender_3c = physical_status(target, 0x3C);
    hit.modifiers.defender_equipment_16 = resources.equipment_special(target, 16);
    hit.defender_3d = physical_status(target, 0x3D);
    hit.counter_reduction_roll = roll(4);
    hit.random_multiplier = ai_config_[2] + roll(ai_config_[3] + 3);
    if (!hit_succeeds(hit, percent())) { return 0; }
    CounterDamageInput counter{};
    auto& damage = counter.physical;
    damage.attack = actor.attack;
    damage.defense = target.defense;
    damage.height_difference = height;
    damage.attack_mode = context.mode;
    damage.context_bonus = hit.context_bonus;
    auto at = terrain_effect(resources, actor, { actor.x, actor.y });
    auto dt = terrain_effect(resources, target, { target.x, target.y });
    damage.attacker_terrain_bonus = simple ? 0 : int(int8_t(at.bytes[1]));
    damage.defender_terrain_bonus = simple ? 0 : int(int8_t(dt.bytes[2]));
    int chance = percent(), decimal = roll(10);
    damage.divisor = damage_divisor(chance, decimal);
    counter.reduction_roll = roll(4);
    damage.modifiers.action_coefficient = action.fields[11];
    damage.modifiers.class_matchup = resources.class_matchup(actor, target);
    damage.modifiers.combat_value = resources.combat_value(actor);
    damage.modifiers.defender_116 = uint8_t(target.abilities >> 8);
    damage.modifiers.defender_3c = physical_status(target, 0x3C);
    damage.modifiers.defender_3f = physical_status(target, 0x3F);
    damage.modifiers.defender_39 = physical_status(target, 0x39);
    damage.modifiers.attacker_3e = physical_status(actor, 0x3E);
    damage.modifiers.defender_equipment_13 = resources.equipment_special(target, 13);
    damage.attacker_3c = physical_status(actor, 0x3C);
    damage.attacker_118 = uint8_t(actor.abilities >> 24);
    damage.defender_kind = uint8_t(target.kind_attribute);
    counter.fixed_damage = (actor.abilities & 0x400) != 0;
    counter.fixed_value = actor.strikeback;
    counter.equipment_30 = resources.equipment_special(actor, 30);
    damage.random_multiplier = ai_config_[0] + roll(ai_config_[1] + 3);
    damage.player_side = !actor.hostile();
    damage.difficulty_bonus = hit.difficulty_bonus;
    bool minimum_required = false;
    counter.minimum_required = &minimum_required;
    int value = counter_damage(counter);
    if (minimum_required)
    {
        counter.minimum_roll = percent();
        value = counter_damage(counter);
    }
    return value;
}

auto Battle::try_counter_exchange(const std::vector<OriginalEvents::Unit>& units, const PhysicalActionContext& original) -> Result<std::vector<PhysicalExchangeStep>>
{
    std::vector<PhysicalExchangeStep> steps;
    if (!original.counter || !units[original.actor].hp || !units[original.target].hp || (physical_status(units[original.target], 0x39) & 2) || (physical_status_dword(units[original.target]) & 0x2B000))
    {
        return Result<std::vector<PhysicalExchangeStep>>::success(std::move(steps));
    }
    auto baseline = random_;
    auto contexts = try_physical_contexts(*hostile_resources_, units, original.target, original.actor, *original.counter, true);
    if (!contexts) { return Result<std::vector<PhysicalExchangeStep>>::failure(contexts.error()); }
    for (auto& context : contexts.value())
    {
        PhysicalExchangeStep step{ std::move(context) };
        if (step.context.target == original.actor)
        {
            step.context = original;
            step.context.actor_snapshot = units[original.actor];
            step.context.target_snapshot = units[original.target];
            step.reverse = true;
            step.damage = draw_reverse_damage(step.context);
            step.consecutive = draw_physical_consecutive(step.context, true);
            if (step.consecutive) { step.damage += draw_reverse_damage(step.context); }
        }
        else
        {
            const auto& c = step.context;
            step.damage = draw_forward_damage(units, c.actor, c.target, c.action, c.mode);
            step.consecutive = draw_physical_consecutive(c, false);
            if (step.consecutive) { step.damage += draw_forward_damage(units, c.actor, c.target, c.action, c.mode); }
        }
        if (!step.damage) { step.consecutive = false; }
        steps.push_back(std::move(step));
    }
    return Result<std::vector<PhysicalExchangeStep>>::success(std::move(steps));
}

auto Battle::advance_physical_proficiency(OriginalEvents::Unit& unit, const OriginalEvents::ActionRecord& action) -> void
{
    int type = action.fields[0];
    unsigned* value = nullptr;
    if (type >= 2 && type <= 7) { value = &unit.weapon_proficiency[size_t(type - 2)]; }
    else if ((type < 1 || type > 10) && type != 11 && type != 12) { value = &unit.special_proficiency; }
    if (value && *value < 1500) { ++*value; }
}

auto Battle::advance_physical_exchange(std::vector<OriginalEvents::Unit>& units) -> void
{
    if (physical_exchange_ && physical_exchange_->growth_pending) { return; }
    while (physical_exchange_ && !combat_feedback_.active() && !animating_movement())
    {
        auto& exchange = *physical_exchange_;
        if (exchange.death)
        {
            if (!exchange.death_started)
            {
                exchange.death_started = true;
                auto [actor, target] = *exchange.death;
                auto started = physical_death_presenter_(actor, target);
                if (!started || !started.value())
                {
                    auto error = started ? make_error("Original physical death refused") : started.error();
                    rollback_physical_exchange(units);
                    report_error(error);
                }
            }
            return;
        }
        auto& steps = exchange.reverse ? exchange.replies : exchange.forward;
        if (exchange.cursor < steps.size())
        {
            auto& step = steps[exchange.cursor];
            const auto& context = step.context;
            size_t actor = step.reverse ? context.target : context.actor;
            size_t target = step.reverse ? context.actor : context.target;
            const auto& action = step.reverse ? *context.counter : context.action;
            units[actor].direction = facing({ units[actor].x, units[actor].y }, { units[target].x, units[target].y }, units[actor].direction);
            bool present_group = !exchange.group_presented;
            if (present_group)
            {
                exchange.group_presented = true;
                present_attack(units, actor, &action);
                if (std::any_of(steps.begin(), steps.end(), [](const auto& entry)
                        {
                            return entry.consecutive;
                        })
                    && attack_animation_active())
                {
                    auto poses = movement_poses_;
                    movement_poses_.insert(movement_poses_.end(), poses.begin(), poses.end());
                }
            }
            if (present_group && movement_animation_enabled_ && action.index == 160)
            {
                auto source = screen_center({ units[actor].x, units[actor].y }, 0, 0), destination = screen_center({ units[target].x, units[target].y }, 0, 0);
                projectile_ = OriginalEvents::effect_flight(source.x - 32, source.y - 16, destination.x - 32, destination.y - 16, 1);
                projectile_elapsed_ = 0;
            }
            if (present_group && movement_animation_enabled_ && animating_movement()) { return; }
            ++exchange.cursor;
            physical_exchange_diagnostics_.push_back({ actor, target, step.damage, exchange.reverse, step.consecutive });
            int damage = step.damage;
            if (!exchange.group_status_applied)
            {
                exchange.group_status_applied = true;
                for (auto& entry : steps)
                {
                    size_t victim = entry.reverse ? entry.context.actor : entry.context.target;
                    const auto& record = entry.reverse ? *entry.context.counter : entry.context.action;
                    if (entry.damage <= 0 || (units[victim].status_flags & 0x20)) { continue; }
                    size_t source = entry.reverse ? entry.context.target : entry.context.actor;
                    entry.damage = apply_physical_status_tail(units, source, victim, record, entry.damage);
                }
                damage = step.damage;
            }
            const auto& soundRecord = step.reverse ? *step.context.counter : step.context.action;
            physical_sound_ = 111;
            if (damage > 0)
            {
                physical_sound_ = int(int16_t(uint16_t(uint8_t(soundRecord.fields[15])) | uint16_t(uint8_t(soundRecord.fields[16])) << 8));
            }
            physical_feedback(units, actor, target, damage, damage == 0);
            if (movement_animation_enabled_ || !physical_exchange_ || physical_exchange_->death) { return; }
            continue;
        }
        if (!exchange.reverse)
        {
            exchange.reverse = true;
            exchange.counter_cursor = 0;
        }
        if (exchange.counter_cursor < exchange.forward.size() && units[exchange.forward.front().context.actor].hp > 0)
        {
            if (!exchange.replies.empty())
            {
                const auto& context = exchange.forward[exchange.counter_cursor - 1].context;
                advance_physical_proficiency(units[context.target], *context.counter);
                exchange.replies.clear();
            }
            auto reply = try_counter_exchange(units, exchange.forward[exchange.counter_cursor++].context);
            if (!reply)
            {
                auto error = reply.error();
                rollback_physical_exchange(units);
                report_error(error);
                return;
            }
            exchange.replies = std::move(reply).value();
            exchange.cursor = 0;
            exchange.group_presented = false;
            exchange.group_status_applied = false;
            if (!exchange.replies.empty()) { continue; }
            continue;
        }
        if (!exchange.replies.empty())
        {
            const auto& context = exchange.forward[exchange.counter_cursor - 1].context;
            advance_physical_proficiency(units[context.target], *context.counter);
            exchange.replies.clear();
        }
        auto counter_awards = std::move(exchange.counter_awards);
        auto forward_awards = std::move(exchange.forward_awards);
        for (size_t i = 0; i < forward_awards.size(); ++i)
        {
            auto& award = forward_awards[i];
            award.raw_xp = 1;
            if (hostile_resources_ && (award.hp_lost || (units[award.target].status_flags & 0x20)))
            {
                auto actor_snapshot = units[award.actor];
                actor_snapshot.ending_combat_value = actor_snapshot.ending_combat_value.value_or(actor_snapshot.hp);
                auto target_snapshot = units[award.target];
                target_snapshot.ending_combat_value = exchange.forward_targets[i].hp;
                int actor = hostile_resources_->combat_value(actor_snapshot, 1);
                int target = hostile_resources_->combat_value(target_snapshot, 1);
                if (actor <= 0 || actor + target <= 0 || !ai_config_[0xE1])
                {
                    award.raw_xp = 0;
                    continue;
                }
                award.raw_xp = std::max(1, (award.hp_lost * target * ai_config_[0xE5] / (actor + target)) / ai_config_[0xE1]);
                if (!units[award.target].hp) { award.raw_xp += ai_config_[0xE2] * target / actor + 3; }
                if (units[award.target].status_flags & 0x20) { award.raw_xp += int(units[award.target].physical_kill_xp); }
            }
        }
        if (!exchange.forward.empty())
        {
            auto actor = exchange.forward.front().context.actor;
            if (!(units[actor].status_flags & 0x20)) { advance_physical_proficiency(units[actor], exchange.forward.front().context.action); }
        }
        if (physical_loot_commit_)
        {
            auto awards = counter_awards;
            if (exchange.forward.empty() || !(units[exchange.forward.front().context.actor].status_flags & 0x20))
            {
                awards.insert(awards.end(), forward_awards.begin(), forward_awards.end());
            }
            auto committed = physical_loot_commit_(units, awards);
            if (!committed || !committed.value())
            {
                auto error = committed ? make_error("Original physical loot commit refused") : committed.error();
                rollback_physical_exchange(units);
                report_error(error);
                return;
            }
        }
        if (physical_growth_)
        {
            std::vector<PhysicalSettlement> awards = counter_awards;
            for (const auto& award : forward_awards)
            {
                if (!(units[award.actor].status_flags & 0x20)) { awards.push_back(award); }
            }
            exchange.growth_pending = true;
            physical_growth_(std::move(awards), [this, &units](bool commit)
                {
                    if (!commit)
                    {
                        rollback_physical_exchange(units);
                        return;
                    }
                    if (physical_loot_finish_)
                    {
                        auto finished = physical_loot_finish_(true);
                        if (!finished || !finished.value())
                        {
                            auto error = finished ? make_error("Original physical loot publication refused") : finished.error();
                            rollback_physical_exchange(units);
                            report_error(error);
                            return;
                        }
                    }
                    physical_exchange_.reset();
                    if (physical_transaction_finish_) { physical_transaction_finish_(true); }
                });
            return;
        }
        if (physical_loot_finish_)
        {
            auto finished = physical_loot_finish_(true);
            if (!finished || !finished.value())
            {
                auto error = finished ? make_error("Original physical loot publication refused") : finished.error();
                rollback_physical_exchange(units);
                report_error(error);
                return;
            }
        }
        auto deaths = std::move(exchange.deaths);
        physical_exchange_.reset();
        if (physical_transaction_finish_) { physical_transaction_finish_(true); }
        if (death_settlement_ && !physical_death_presenter_)
        {
            for (auto [actor, target] : deaths) { death_settlement_(units, actor, target); }
        }
        if (physical_settlement_)
        {
            auto publish = [&](const auto& awards, bool require_alive)
            {
                std::vector<PhysicalSettlement> totals;
                for (const auto& award : awards)
                {
                    if (award.raw_xp <= 0) { continue; }
                    auto found = std::find_if(totals.begin(), totals.end(), [&](const auto& total)
                        {
                            return total.actor == award.actor;
                        });
                    if (found == totals.end()) { totals.push_back(award); }
                    else
                    {
                        found->raw_xp += award.raw_xp;
                        found->hp_lost += award.hp_lost;
                        found->killed |= award.killed;
                    }
                }
                for (const auto& award : totals)
                {
                    if (!require_alive || !(units[award.actor].status_flags & 0x20)) { physical_settlement_(units, award); }
                }
            };
            publish(counter_awards, false);
            publish(forward_awards, true);
        }
    }
}

auto Battle::rollback_physical_exchange(std::vector<OriginalEvents::Unit>& units) -> void
{
    if (!physical_exchange_) { return; }
    if (physical_loot_finish_) { physical_loot_finish_(false); }
    auto& baseline = physical_exchange_->baseline_units;
    if (units.size() == baseline.size()) { std::copy(baseline.begin(), baseline.end(), units.begin()); }
    else
    {
        units = baseline;
    }
    random_ = physical_exchange_->baseline_random;
    physical_exchange_.reset();
    physical_exchange_diagnostics_.clear();
    pending_action_status_.reset();
    physical_sound_ = 0;
    if (physical_transaction_finish_) { physical_transaction_finish_(false); }
}

auto Battle::strike(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot) -> bool
{
    if (a >= units.size() || b >= units.size() || a == b || animating_movement() || combat_feedback_active())
    {
        return false;
    }
    auto& attacker = units[a];
    auto& defender = units[b];
    if (!can_attack(units, a, b, slot))
    {
        return false;
    }
    auto action = executable_action(units, a, b, slot);
    if (original_rules_ && attacker.action_points < attack_cost(attacker))
    {
        return false;
    }
    if (original_rules_ && action && hostile_resources_)
    {
        auto baseline_random = random_;
        auto planned = try_forward_exchange(*hostile_resources_, units, a, b, *action);
        if (!planned)
        {
            report_error(planned.error());
            return false;
        }
        auto baseline_units = units;
        int cost = attack_cost(attacker);
        attacker.action_points = int(int16_t(attacker.action_points - cost));
        attacker.max_action_points = attacker.action_points;
        attacker.carry_points = std::min(attacker.carry_points, attacker.action_points);
        attacker.status34 |= 2;
        physical_exchange_diagnostics_.clear();
        physical_exchange_.emplace();
        physical_exchange_->baseline_units = std::move(baseline_units);
        physical_exchange_->baseline_random = baseline_random;
        physical_exchange_->forward = std::move(planned).value();
        advance_physical_exchange(units);
        return true;
    }
    attacker.direction = facing({ attacker.x, attacker.y }, { defender.x, defender.y }, attacker.direction);
    present_attack(units, a, action ? &*action : nullptr);
    if (movement_animation_enabled_ && action && action->index == 160)
    {
        auto source = screen_center({ attacker.x, attacker.y }, 0, 0), target = screen_center({ defender.x, defender.y }, 0, 0);
        projectile_ = OriginalEvents::effect_flight(source.x - 32, source.y - 16, target.x - 32, target.y - 16, 1);
        projectile_elapsed_ = 0;
    }
    if (original_rules_)
    {
        int cost = attack_cost(attacker);
        if (attacker.action_points < cost)
        {
            return false;
        }
        attacker.action_points = int(int16_t(attacker.action_points - cost));
        attacker.max_action_points = attacker.action_points;
        attacker.carry_points = std::min(attacker.carry_points, attacker.action_points);
        attacker.status34 |= 2;
    }
    bool simple_rules = original_rules_ && hostile_resources_ && hostile_resources_->difficulty == 1;
    int attack_style = attack_mode({ attacker.x, attacker.y }, { defender.x, defender.y }, defender.direction, simple_rules);
    if (original_rules_ && action)
    {
        int dealt = draw_forward_damage(units, a, b, *action, attack_style);
        if (!dealt)
        {
            physical_feedback(units, a, b, 0, true);
            message_ = "Original physical action: miss";
            return true;
        }
        if (action->fields[0] == 40)
        {
            if (movement_animation_enabled_)
            {
                pending_action_status_ = *action;
            }
            else
            {
                dealt = apply_physical_action_status(units, b, *action, dealt);
            }
        }
        physical_feedback(units, a, b, dealt);
        message_ = "Original physical action: -" + std::to_string(dealt) + " HP; target HP " + std::to_string(defender.hp);
        return true;
    }
    int hitBonus;
    if (attack_style == 3) { hitBonus = 5; }
    else if (attack_style == 2) { hitBonus = 3; }
    else
    {
        hitBonus = 0;
    }
    int hit = std::clamp(attacker.tactics - defender.strikeback + (hitBonus),
                  1, 100)
        * 12 / 10;
    if (percent() > hit)
    {
        physical_feedback(units, a, b, 0, true);
        message_ = "Ordinary melee: miss";
        return true;
    }
    int chance = percent(), jitter = roll(10);
    int divisor = s2ne::damage_divisor(chance, jitter);
    int minimum_roll = (attacker.attack - defender.defense) * 100 / divisor < 3 ? percent() : 0;
    int attackBonus;
    if (attack_style == 3) { attackBonus = 10; }
    else if (attack_style == 2) { attackBonus = 5; }
    else
    {
        attackBonus = 0;
    }
    int damage = ordinary_damage(attacker.attack + (attackBonus),
        defender.defense, divisor, minimum_roll);
    physical_feedback(units, a, b, damage);
    message_ = "Ordinary melee: -" + std::to_string(damage) + " HP; target HP " + std::to_string(defender.hp);
    return true;
}

auto Battle::click(std::vector<OriginalEvents::Unit>& units, Point p) -> bool
{
    if (teleport_pending_ || animating_movement() || combat_feedback_active())
    {
        return false;
    }
    if (!valid(p))
    {
        return false;
    }
    int hit = occupant(units, p);
    if (state_ == State::idle)
    {
        return hit >= 0 && select_or_inspect_unit(units, size_t(hit));
    }
    if (state_ == State::movement)
    {
        build_routes(units);
        if (!preview(p))
        {
            message_ = "Destination unreachable or occupied";
            return false;
        }
        auto route = path_;
        auto saved = units[selected_];
        movement_poses_.clear();
        unsigned movement_class = 0;
        if (hostile_resources_) { movement_class = hostile_resources_->movement_class(saved); }
        else if (size_t(selected_) < movement_contexts_.size()) { movement_class = movement_contexts_[selected_].movement_class; }
        // 452AD5 routes classes 3/4/5 through 452450's 12-centisecond wait.
        movement_interval_ = movement_class >= 3 && movement_class <= 5 ? 120 : 80;
        for (auto q : route)
        {
            if (!step(units, selected_, q, true))
            {
                units[selected_] = saved;
                movement_poses_.clear();
                return false;
            }
            auto pose = units[selected_];
            pose.movement_pose = true;
            if (movement_class >= 3 && movement_class <= 5)
            {
                pose.movement_visual_class = movement_class;
                pose.movement_camera_pose = true;
            }
            const auto& previous_pose = movement_poses_.empty() ? saved : movement_poses_.back();
            unsigned int movementFrame = 0;
            if (previous_pose.direction == pose.direction)
            {
                movementFrame = previous_pose.movement_frame;
                if (movement_class != 4 && movement_class != 5) { ++movementFrame; }
            }

            pose.movement_frame = movementFrame;
            movement_poses_.push_back(std::move(pose));
        }
        if (!movement_poses_.empty())
        {
            units[selected_].movement_frame = movement_poses_.back().movement_frame;
        }
        if (movement_class == 2)
        {
            auto& actor = units[selected_];
            actor.direction = saved.direction;
            actor.movement_frame = saved.movement_frame;
            actor.movement_pose = false;
            movement_poses_.clear();
            if (teleport_presenter_)
            {
                const Point destination{ actor.x, actor.y };
                actor.x = saved.x;
                actor.y = saved.y;
                teleport_pending_ = teleport_movement_ = true;
                teleport_destination_ = destination;
                if (!teleport_presenter_(unsigned(selected_), destination))
                {
                    rollback_teleport();
                    actor = saved;
                    return false;
                }
            }
        }
        // 452450+2D0 skips destination AP writeback for class5.
        if (movement_class == 5)
        {
            auto preserve_budget = [&](auto& unit)
            {
                unit.action_points = saved.action_points;
                unit.max_action_points = saved.max_action_points;
                unit.carry_points = saved.carry_points;
            };
            preserve_budget(units[selected_]);
            for (auto& pose : movement_poses_)
            {
                preserve_budget(pose);
            }
        }
        if (movement_animation_enabled_ && !movement_poses_.empty())
        {
            moving_unit_ = selected_;
            movement_presented_ = false;
            movement_elapsed_ = 0;
        }
        moved_ = true;
        state_ = State::command;
        path_.clear();
        message_ = "Moved; choose attack or finish, or cancel to undo";
        return true;
    }
    if (state_ == State::attack_target)
    {
        hit = attack_target_at(units, p);
        if (hit >= 0)
        {
            target_ = hit;
            return activate(units);
        }
    }
    return false;
}

auto Battle::has_movement_mouse() const -> bool { return state_ == State::movement && mouse_position_.x >= 0 && !movement_mouse_[0].empty(); }

auto Battle::draw_movement_mouse(std::vector<uint8_t>& image) const -> bool
{
    if (!has_movement_mouse() || effect_palette_.size() < 768)
    {
        return false;
    }
    unsigned sprite = 0;
    if (valid(hovered_cell_))
    {
        size_t cell = size_t(hovered_cell_.y) * width_ + hovered_cell_.x;
        if (cell < previous_.size() && previous_[cell] >= 0)
        {
            sprite = 1 + movement_categories_[cell];
        }
    }
    const auto& pixels = movement_mouse_[sprite];
    auto size = movement_mouse_sizes_[sprite];
    if (pixels.empty())
    {
        return false;
    }
    for (int y = 0; y < size.y; ++y)
    {
        for (int x = 0; x < size.x; ++x)
        {
            unsigned c = pixels[size_t(y) * size.x + x];
            int dx = mouse_position_.x + x, dy = mouse_position_.y + y;
            if (!c || dx < 0 || dy < 0 || dx >= 640 || dy >= 480)
            {
                continue;
            }
            size_t p = (size_t(dy) * 640 + dx) * 4;
            for (unsigned k = 0; k < 3; ++k)
            {
                image.at(p + k) = uint8_t(unsigned(effect_palette_[c * 3 + k]) * 255 / 63);
            }
        }
    }
    return true;
}

auto Battle::draw_finish_directions(std::vector<uint8_t>& image, int left, int top, unsigned direction) const -> void
{
    if (image.size() < 640 * 480 * 4 || effect_palette_.size() < 768)
    {
        return;
    }
    auto blit = [&](unsigned slot, int dx, int dy)
    {
        const auto& sprite = finish_directions_[slot];
        for (size_t i = 0; i < sprite.pixels.size(); ++i)
        {
            unsigned color = sprite.pixels[i];
            int x = dx + int(i % sprite.width), y = dy + int(i / sprite.width);
            if (!color || x < 0 || x >= 640 || y < 0 || y >= 480)
            {
                continue;
            }
            size_t destination = (size_t(y) * 640 + x) * 4;
            for (unsigned channel = 0; channel < 3; ++channel)
            {
                image[destination + channel] = uint8_t(unsigned(effect_palette_[color * 3 + channel]) * 255 / 63);
            }
        }
    };
    blit(0, left, top);
    if (direction < 4)
    {
        constexpr std::array<Point, 4> offsets{ { { 0, 36 }, { 63, 36 }, { 60, 0 }, { 0, 0 } } };
        auto offset = offsets[direction];
        blit(direction + 1, left + offset.x, top + offset.y);
    }
}

auto Battle::draw_projectile(std::vector<uint8_t>& image, int cx, int cy) const -> void
{
    if (image.size() < 640 * 480 * 4 || effect_palette_.size() < 768 || !projectile_ || animating_movement())
    {
        return;
    }
    const auto& f = *projectile_;
    const auto& sprite = projectile59_.at(f.direction);
    for (unsigned y = 0; y < sprite.height; ++y)
    {
        for (unsigned x = 0; x < sprite.width; ++x)
        {
            unsigned color = sprite.pixels[size_t(y) * sprite.width + x];
            int dx = f.x - cx + int(x), dy = f.y - cy + int(y);
            if (!color || dx < 0 || dy < 0 || dx >= 640 || dy >= 480)
            {
                continue;
            }
            size_t p = (size_t(dy) * 640 + dx) * 4;
            for (unsigned k = 0; k < 3; ++k)
            {
                image[p + k] = uint8_t(unsigned(effect_palette_[color * 3 + k]) * 255 / 63);
            }
        }
    }
}

auto Battle::draw_skill_cells(std::vector<uint8_t>& image, int cx, int cy, unsigned category, const std::function<bool(Point)>& includes, const std::function<void(std::vector<uint8_t>&, Point)>& mask) const -> void
{
    const auto& tile = movement_effects_.at(category);
    for (int y = 0; y < height_; ++y)
    {
        for (int x = 0; x < width_; ++x)
        {
            if (!skill_area_cell({ x, y }) || !includes({ x, y })) { continue; }
            auto pixels = tile;
            if (mask) { mask(pixels, { x, y }); }
            auto point = screen_center({ x, y }, cx, cy);
            for (int row = 0; row < 32; ++row)
            {
                for (int col = 0; col < 64; ++col)
                {
                    unsigned color = pixels[size_t(row) * 64 + col];
                    int px = point.x - 32 + col, py = point.y - 16 + row;
                    if (!color || px < 0 || px >= 640 || py < 120 || py >= 480) { continue; }
                    auto dst = (size_t(py) * 640 + px) * 4;
                    for (unsigned k = 0; k < 3; ++k) { image[dst + k] = uint8_t(unsigned(effect_palette_[color * 3 + k]) * 255 / 63); }
                }
            }
        }
    }
}

auto Battle::draw_selection(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, int cx, int cy, int layer, const std::function<void(std::vector<uint8_t>&, Point)>& mask) const -> void
{
    if (image.size() < 640 * 480 * 4 || effect_palette_.size() < 768)
    {
        return;
    }
    if (layer != 0)
    {
        draw_projectile(image, cx, cy);
    }
    auto blit = [&](const std::vector<uint8_t>& tile, Point cell)
    {
        if (tile.empty())
        {
            return;
        }
        auto clipped = tile;
        if (mask) { mask(clipped, cell); }
        auto c = screen_center(cell, cx, cy);
        int height = int(tile.size() / 64);
        // sub_42B0C6 bottom-aligns both 32- and 137-row resources.
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < 64; ++x)
            {
                unsigned color = clipped[size_t(y) * 64 + x];
                int dx = c.x - 32 + x, dy = c.y + 16 - height + y;
                if (!color || dx < 0 || dy < 0 || dx >= 640 || dy >= 480)
                {
                    continue;
                }
                size_t dst = (size_t(dy) * 640 + dx) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    image[dst + k] = uint8_t(unsigned(effect_palette_[color * 3 + k]) * 255 / 63);
                }
            }
        }
    };
    if (state_ == State::movement && layer != 1)
    {
        for (size_t i = 0; i < distances_.size(); ++i)
        {
            if (previous_[i] < 0)
            {
                continue;
            }
            blit(movement_effects_[movement_categories_[i]], { int(i % width_), int(i / width_) });
        }
    }
    Point cursor = hovered_cell_;
    if (state_ == State::attack_target && target_ >= 0 && size_t(target_) < units.size())
    {
        cursor = { units[target_].x, units[target_].y };
    }
    else if (state_ != State::movement && selected_ >= 0 && size_t(selected_) < units.size())
    {
        cursor = { units[selected_].x, units[selected_].y };
    }
    if (!valid(cursor))
    {
        return;
    }
    if (selected_ < 0)
    {
        auto surface = cell(cursor);
        if (surface.elevation <= 0 || surface.elevation > 20 || surface.ground == 1)
        {
            return;
        }
        bool simple = std::find(sealed_cells_.begin(), sealed_cells_.end(), cursor) != sealed_cells_.end();
        if (simple)
        {
            if (layer != 1)
            {
                blit(ground_cursor_, cursor);
            }
            return;
        }
    }
    // Original back/front cursor masks; scene occlusion belongs to the scene renderer.
    if (layer < 0)
    {
        for (const auto& frame : selection_frames_)
        {
            blit(frame, cursor);
        }
    }
    else if (layer < 2)
    {
        blit(selection_frames_[size_t(layer)], cursor);
    }
}
}    //namespace s2ne
