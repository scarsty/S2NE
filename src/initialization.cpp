#include "initialization.h"

namespace s2ne
{
auto Initialization::get(const std::vector<uint8_t>& b, size_t p, unsigned n) -> unsigned
{
    unsigned v = 0;
    for (unsigned i = 0; i < n; ++i)
    {
        v |= unsigned(b.at(p + i)) << (8 * i);
    }
    return v;
}

auto Initialization::entry(unsigned id) const -> size_t { return get(pal_, 848 + (id - 1) * 8, 4) - 11 - (id - 1); }

auto Initialization::try_text(unsigned id) const -> Result<std::vector<uint8_t>>
{
    ResourceReader pal(pal_), bin(bin_);
    unsigned raw = pal.number(880, 4);
    if (!pal)
    {
        return Result<std::vector<uint8_t>>::failure(pal.error());
    }
    if (raw < 15)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid save label table"));
    }
    size_t table = raw - 15;
    unsigned count = bin.number(table);
    if (id >= count || !bin.extent(table + 2, size_t(count) * 8))
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid save label string"));
    }
    size_t base = table + 2 + size_t(count) * 8, offset = bin.number(table + 2 + id * 8, 4);
    if (base > bin_.size() || offset > bin_.size() - base)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid label extent"));
    }
    size_t begin = base + offset, end = begin;
    while (end < bin_.size() && bin_[end])
    {
        ++end;
    }
    if (end == bin_.size())
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Unterminated save label"));
    }
    return Result<std::vector<uint8_t>>::success({ bin_.begin() + begin, bin_.begin() + end });
}

auto Initialization::try_describe(Save::State& raw, const Events& events) const -> Result<bool>
{
    auto labelResult = try_text(144), turnResult = try_text(1);
    if (!labelResult)
    {
        return Result<bool>::failure(labelResult.error());
    }
    if (!turnResult)
    {
        return Result<bool>::failure(turnResult.error());
    }
    auto label = std::move(labelResult).value(), turn = std::move(turnResult).value();
    std::vector<uint8_t> bytes = label;
    bytes.push_back(' ');
    auto number = std::to_string(SaveBridge::get(raw.stage_4fb478, 8, 4));
    bytes.insert(bytes.end(), number.begin(), number.end());
    bytes.push_back('.');
    if (events.stage_name.size() <= 40)
    {
        bytes.insert(bytes.end(), events.stage_name.begin(), events.stage_name.end());
    }
    while (bytes.size() < 25)
    {
        bytes.push_back(' ');
    }
    bool substituted = false;
    for (size_t i = 0; i < turn.size(); ++i)
    {
        if (turn[i] == '%' && i + 2 < turn.size() && turn[i + 1] == '2' && turn[i + 2] == 'd')
        {
            auto n = std::to_string(SaveBridge::get(raw.globals_4f7cd0, 12, 4));
            if (n.size() < 2)
            {
                bytes.push_back(' ');
            }
            bytes.insert(bytes.end(), n.begin(), n.end());
            i += 2;
            substituted = true;
        }
        else
        {
            bytes.push_back(turn[i]);
        }
    }
    if (!substituted || bytes.size() >= raw.description.size())
    {
        return Result<bool>::failure(make_error("Original save label exceeds50 bytes"));
    }
    raw.description.fill(0);
    std::copy(bytes.begin(), bytes.end(), raw.description.begin());
    constexpr char attribution[] = "The SuperSp Game V1.3d Copyright (c) 1997 by U&J TECHNOLOGY CO., LTD.";
    raw.attribution.fill(0);
    std::copy_n(attribution, sizeof(attribution), raw.attribution.begin());
    return Result<bool>::success(true);
}

auto Initialization::Source::id() const -> unsigned { return bytes[0] | (unsigned(bytes[1]) << 8); }

auto Initialization::Source::behavior() const -> int { return int(int8_t(bytes[49])); }

auto Initialization::Source::first_turn() const -> int { return int(int8_t(bytes[51])); }

auto Initialization::Source::last_turn() const -> int { return int(int8_t(bytes[52])); }

auto Initialization::Source::arrives(unsigned turn) const -> bool
{
    if (!turn || turn > 0x7FFFFFFF)
    {
        return false;
    }
    return last_turn() == 0 ? first_turn() == int(turn) : first_turn() <= int(turn) && last_turn() >= int(turn);
}

auto Initialization::Source::control_class() const -> int
{
    int b = behavior();
    return b == 0 || b == 5 || b == 6 || b == 7 ? 0 : 1;
}

auto Initialization::Source::role6() const -> int { return int(int8_t(bytes[21])); }

auto Initialization::Source::role_a() const -> int { return int(int8_t(bytes[20])); }

auto Initialization::Source::role_c() const -> int { return int(int8_t(bytes[22])); }

auto Initialization::PlacementMap::accepts(int x, int y) const -> bool
{
    if (x < 0 || y < 0 || (!(y & 1) && x < 1) || x >= int(width) - 1 || y >= int(height) - 1)
    {
        return false;
    }
    int elevation = elevations.at(size_t(y) * width + unsigned(x));
    return y - (elevation > 0 ? elevation - 1 : 0) >= 1 && std::find(sealed.begin(), sealed.end(), std::pair{ x, y }) == sealed.end();
}

auto Initialization::try_placement_map(const std::filesystem::path& root, unsigned map) const -> Result<PlacementMap>
{
    auto loaded = read_shared_resource(root / "SUPERSP.PRG");
    if (!loaded)
    {
        return Result<PlacementMap>::failure(loaded.error());
    }
    const auto& bytes = loaded.value();
    ResourceReader pal(pal_), prg(bytes);
    unsigned raw = pal.number(808 + 8 * (64 + 6 - 1), 4);
    if (!pal)
    {
        return Result<PlacementMap>::failure(pal.error());
    }
    if (raw < 15)
    {
        return Result<PlacementMap>::failure(make_error("Invalid placement map address"));
    }
    size_t table = raw - 15;
    unsigned count = prg.number(table);
    if (!map || map > count)
    {
        return Result<PlacementMap>::failure(make_error("Invalid original placement map"));
    }
    size_t offset = prg.number(table + 2 + size_t(map - 1) * 8, 4);
    if (!prg)
    {
        return Result<PlacementMap>::failure(prg.error());
    }
    if (table > bytes.size() || offset > bytes.size() - table)
    {
        return Result<PlacementMap>::failure(make_error("Invalid placement extent"));
    }
    size_t m = table + offset;
    PlacementMap result;
    result.width = prg.number(m);
    result.height = prg.number(m + 2) * 2;
    if (!result.width || !result.height)
    {
        return Result<PlacementMap>::failure(make_error("Invalid placement dimensions"));
    }
    size_t cells = size_t(result.width) * result.height;
    if (!prg.extent(m + 28, cells * 5))
    {
        return Result<PlacementMap>::failure(prg.error());
    }
    for (size_t i = 0; i < cells; ++i)
    {
        result.elevations.push_back(bytes[m + 28 + i * 5]);
    }
    size_t special = m + 28 + cells * 5 + prg.number(m + 12) * 10;
    unsigned specials = prg.number(m + 14);
    if (!prg.extent(special, size_t(specials) * 6))
    {
        return Result<PlacementMap>::failure(prg.error());
    }
    for (unsigned i = 0; i < specials; ++i)
    {
        size_t p = special + i * 6;
        if (prg.number(p + 4) & 0x8000)
        {
            result.sealed.emplace_back(int(int16_t(prg.number(p))), int(int16_t(prg.number(p + 2))));
        }
    }
    return Result<PlacementMap>::success(std::move(result));
}

auto Initialization::placement_map(const std::filesystem::path& root, unsigned map) const -> PlacementMap
{
    auto result = try_placement_map(root, map);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::place(int x, int y, const PlacementMap& map, std::span<const Save::UnitRecord> first, std::span<const Save::UnitRecord> second) -> std::pair<int, int>
{
    auto occupied = [&](int cx, int cy)
    {
        for (auto army : { first, second })
        {
            for (const auto& r : army)
            {
                unsigned status = r[0x33];
                if (!(status & 0x80) || ((status & 0x20) && !(status & 2)))
                {
                    continue;
                }
                if (int(int32_t(SaveBridge::get(r, 0x23, 4))) == cx && int(int32_t(SaveBridge::get(r, 0x27, 4))) == cy)
                {
                    return true;
                }
            }
        }
        return false;
    };
    if (!occupied(x, y))
    {
        return { x, y };
    }
    auto accepts = [&](int cx, int cy)
    {
        return map.accepts(cx, cy) && !occupied(cx, cy);
    };
    int left = x, top = y;
    for (int radius = 1; radius <= 10; ++radius)
    {
        if (!(top & 1))
        {
            --left;
        }
        --top;
        for (int cx = left; cx <= left + radius; ++cx)
        {
            if (accepts(cx, top))
            {
                return { cx, top };
            }
            if (accepts(cx, top + 2 * radius))
            {
                return { cx, top + 2 * radius };
            }
        }
        for (int cy = top + 2; cy <= top + 2 * radius - 2; cy += 2)
        {
            if (accepts(left, cy))
            {
                return { left, cy };
            }
            if (accepts(left + radius, cy))
            {
                return { left + radius, cy };
            }
        }
    }
    return { x, y };
}

auto Initialization::create(const std::filesystem::path& root) -> Result<Initialization>
{
    auto events = Events::create(root, 3);
    if (!events)
    {
        return Result<Initialization>::failure(events.error());
    }
    Initialization result;
    result.root_ = root;
    for (auto pair : { std::pair{ &result.bin_, "SUPERSP.BIN" }, std::pair{ &result.pal_, "SUPERSP.PAL" }, std::pair{ &result.prg_, "SUPERSP.PRG" } })
    {
        auto bytes = read_shared_resource(root / pair.second);
        if (!bytes)
        {
            return Result<Initialization>::failure(bytes.error());
        }
        *pair.first = std::move(bytes).value();
    }
    auto items = read_resource(root / "ITEMSP.BIN");
    if (!items) { return Result<Initialization>::failure(items.error()); }
    result.items_ = std::move(items).value();
    return Result<Initialization>::success(std::move(result));
}

Initialization::Initialization(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto Initialization::opening() const -> Opening
{
    size_t p = entry(10);
    p += 2 + 4 * get(bin_, p);
    p += 2 + get(bin_, p);
    p += 2;
    return { bin_.at(p + 0xFE), bin_.at(p + 0xFF), bin_.at(p + 0x100), int(int8_t(bin_.at(p + 0xF9))), int(int8_t(bin_.at(p + 0xFA))) };
}

auto Initialization::empty_group() const -> Save::Group
{
    Save::Group g;
    g.inventory.resize(Save::item_count(items_));
    for (unsigned i = 0; i < 11; ++i)
    {
        g.status[1 + i * 3] = uint8_t(113 + i);
    }
    return g;
}

auto Initialization::try_summon_source(unsigned army, unsigned appearance, int level, int x, int y,
    RandomStream& random, const Promotion& promotion, const RandomStream::TimeSource& time) const -> Result<Source>
{
    const auto kinds = read_resource(root_ / "KINDSP.BIN");
    if (!kinds) { return Result<Source>::failure(kinds.error()); }
    ResourceReader reader(kinds.value());
    const unsigned count = reader.number(4), records = reader.number(18, 4);
    const unsigned growth = get(pal_, 904, 4);
    if (army > 1 || !appearance || appearance > count || level < 1 || level > 10 || growth < 18
        || x < -32768 || x > 32767 || y < -32768 || y > 32767)
    {
        return Result<Source>::failure(make_error("Invalid44494E summon initialization"));
    }
    const size_t base = size_t(growth - 18) + 2 + size_t(count) * 108 + size_t(appearance - 1) * 50;
    if (base > bin_.size() || 50 > bin_.size() - base || !reader.extent(records + size_t(appearance - 1) * 37, 37))
    {
        return Result<Source>::failure(make_error("Truncated42788A summon base record"));
    }
    Source source;
    source.group = army;
    const size_t kind = records + size_t(appearance - 1) * 37;
    for (unsigned i = 0; i < 14 && kinds.value().at(kind + i); ++i) { source.bytes[2 + i] = kinds.value().at(kind + i); }
    auto put = [&](unsigned offset, int value)
    {
        source.bytes.at(offset) = uint8_t(value);
        source.bytes.at(offset + 1) = uint8_t(uint16_t(value) >> 8);
    };
    put(16, appearance);
    put(18, level);
    put(23, x);
    put(25, y);
    source.bytes[49] = uint8_t(army);
    source.bytes[50] = army == 0 ? 1 : 0;
    source.bytes[51] = 1;
    constexpr unsigned offsets[]{ 27, 29, 31, 37, 39, 35, 43, 33, 41 };
    for (unsigned i = 0; i < 9; ++i) { put(offsets[i], int16_t(get(bin_, base + i * 2))); }
    for (unsigned i = 0; i < 5; ++i) { put(63 + i * 2, get(bin_, base + 18 + i * 2)); }
    for (unsigned i = 0; i < 11; ++i) { put(79 + i * 2, get(bin_, base + 28 + i * 2)); }
    put(45, kinds.value().at(kind + 26));
    put(47, int8_t(kinds.value().at(kind + 21)));
    auto next = random;
    for (int current = 2; current <= level; ++current)
    {
        auto increase = promotion.try_growth(appearance, current, next, time);
        if (!increase) { return Result<Source>::failure(increase.error()); }
        for (unsigned i = 0; i < 9; ++i)
        {
            const unsigned offset = offsets[i];
            int value = int16_t(unsigned(source.bytes[offset]) | unsigned(source.bytes[offset + 1]) << 8);
            value = int16_t(value + increase.value()[i]);
            put(offset, std::clamp(value, 0, i == 0 ? 5000 : 999));
        }
    }
    random = next;
    return Result<Source>::success(std::move(source));
}

auto Initialization::try_new_summon_role(const Source& source, unsigned index, unsigned control,
    const Events& events, const PlacementMap& map, std::span<const Save::UnitRecord> first,
    std::span<const Save::UnitRecord> second, unsigned game_mode) const -> Result<Save::UnitRecord>
{
    if (source.id() || source.group > 1 || control > 1)
    {
        return Result<Save::UnitRecord>::failure(make_error("Unsupported44494E summon owner/army adjustment"));
    }
    const auto loaded = read_resource(root_ / "KINDSP.BIN");
    if (!loaded) { return Result<Save::UnitRecord>::failure(loaded.error()); }
    const auto& kinds = loaded.value();
    auto word = [&](unsigned offset)
    {
        return unsigned(source.bytes.at(offset)) | unsigned(source.bytes.at(offset + 1)) << 8;
    };
    const unsigned appearance = word(16);
    if (!appearance || appearance > get(kinds, 4)) { return Result<Save::UnitRecord>::failure(make_error("Invalid44494E summon appearance")); }
    const size_t kind = get(kinds, 18, 4) + size_t(appearance - 1) * 37;
    Events::Unit unit;
    unit.id = 0;
    unit.appearance = appearance;
    unit.level = word(18);
    unit.x = int16_t(word(23));
    unit.y = int16_t(word(25));
    unit.behavior = 0;
    if (source.group) { unit.behavior = 2; }
    else if (control) { unit.behavior = 1; }
    unit.status_flags = 0x90;
    unit.runtime_flags30 = 0x90000000;
    unit.direction = source.bytes[50];
    unit.first_turn = 1;
    unit.hp = unit.max_hp = int16_t(word(27));
    unit.mp = unit.max_mp = int16_t(word(29));
    unit.st = unit.max_st = int16_t(word(31));
    unit.sp = unit.max_sp = int16_t(word(33));
    unit.strikeback = int16_t(word(35));
    unit.attack = unit.attack_limit = int16_t(word(37));
    unit.defense = unit.defense_limit = int16_t(word(39));
    unit.fencing = int16_t(word(41));
    unit.tactics = unit.base_tactics = int16_t(word(43));
    unit.kind_attribute = uint8_t(word(45));
    unit.base_action_points = unit.action_points = unit.max_action_points = int16_t(word(47));
    unit.experience_threshold = get(kinds, kind + 14);
    if (kinds.at(kind + 19)) { unit.abilities = 1u << ((kinds.at(kind + 19) - 1) & 31); }
    for (unsigned p = 2; p < 16 && source.bytes[p]; ++p) { unit.name.push_back(source.bytes[p]); }
    const size_t directory = entry(2);
    size_t ability = directory + 9 + 11 * 2;
    for (unsigned group = 0; group < 11; ++group)
    {
        unit.learned[group] = word(79 + group * 2);
        const unsigned count = bin_.at(directory + 10 + group * 2);
        for (unsigned bit = 0; bit < count; ++bit, ability += 37)
        {
            if (!(unit.learned[group] & (1u << (bit & 31)))) { continue; }
            const int threshold = int16_t(get(bin_, ability + 15));
            if (int(unit.learned_proficiency[group] / 10) < threshold) { unit.learned_proficiency[group] = uint16_t(threshold * 10); }
        }
    }
    PromotionTransaction equipment(root_);
    PromotionTransaction::State state{ unit, std::vector<uint8_t>(equipment.item_count()), RandomStream({ 1, 0 }) };
    for (unsigned i = 0; i < 6; ++i)
    {
        const unsigned item = word(63 + i * 2);
        if (!item) { continue; }
        if (item > state.inventory.size()) { return Result<Save::UnitRecord>::failure(make_error("Invalid summon source equipment")); }
        state.inventory[item - 1] = 1;
        if (!equipment.equip(state, item)) { return Result<Save::UnitRecord>::failure(make_error("Unsupported summon source item distribution")); }
    }
    unit = std::move(state.unit);
    if (source.group && game_mode != 1)
    {
        const size_t stage = entry(1) + 2 + size_t(original_stage_id(events.stage) - 1) * 30;
        if (bin_.at(stage + 21) != 3)
        {
            size_t config = entry(10);
            config += 2 + 4 * get(bin_, config);
            config += 2 + get(bin_, config);
            config += 2;
            const int factor = kinds.at(kind + 20);
            const unsigned minimum = unsigned(std::clamp(int(int8_t(bin_.at(config + 0xE0))) * int(events.difficulty) * factor / 10 + unit.level * factor / 20, 0, 999));
            for (const auto& action : events.action_records(unit))
            {
                const int type = action.fields[0];
                if (type >= 2 && type <= 7) { unit.weapon_proficiency[type - 2] = std::max(unit.weapon_proficiency[type - 2], minimum); }
                else if ((type < 1 || type > 10) && type != 11 && type != 12) { unit.special_proficiency = std::max(unit.special_proficiency, minimum); }
            }
        }
    }
    auto record = try_new_role(source, unit, index, map, first, second);
    if (record)
    {
        SaveBridge::put(record.value(), 8, control);
        if (source.group)
        {
            auto adjusted = s2ne::try_adjust_runtime_stats(record.value(), events.stage_adjustment);
            if (!adjusted) { return Result<Save::UnitRecord>::failure(adjusted.error()); }
        }
    }
    return record;
}

auto Initialization::try_summon_role(const Save::State& before, const Events& events,
    unsigned army, unsigned slot, int level, int x, int y, unsigned control, RandomStream& random,
    const Promotion& promotion, const RandomStream::TimeSource& time) const -> Result<Save::State>
{
    if (army > 1 || slot >= 11 || control > 1 || level < 1 || level > 10 || events.stage != SaveBridge::get(before.globals_4f7cd0, 8, 4))
    {
        return Result<Save::State>::failure(make_error("Invalid44494E summon transaction"));
    }
    auto staged = before;
    auto& group = staged.groups[army];
    const unsigned appearance = group.status[slot * 3 + 1] | unsigned(group.status[slot * 3 + 2]) << 8;
    if (!appearance || group.status[slot * 3]) { return Result<Save::State>::failure(make_error("Original summon slot already deployed/undefined")); }
    for (unsigned i = 0; i < 11; ++i)
    {
        if (group.status[i * 3] == 1) { return Result<Save::State>::failure(make_error("44CB0E army already has a deployed summon")); }
    }
    auto next = random;
    auto map = try_placement_map(root_, events.map_id);
    if (!map) { return Result<Save::State>::failure(map.error()); }
    auto existing = std::find_if(group.units.begin(), group.units.end(), [&](const auto& record)
        {
            return (unsigned(record[4]) | unsigned(record[5]) << 8) == appearance && (record[0x33] & 0x10);
        });
    if (existing == group.units.end())
    {
        if (group.units.size() >= Save::maximum_units) { return Result<Save::State>::failure(make_error("Original summon army full")); }
        if (SaveBridge::get(before.globals_4f7cd0, 0, 4) == 1 || army == 0) { level = 1; }
        auto source = try_summon_source(army, appearance, level, x, y, next, promotion, time);
        if (!source) { return Result<Save::State>::failure(source.error()); }
        auto role = try_new_summon_role(source.value(), unsigned(group.units.size() + 1), control, events, map.value(), staged.groups[0].units, staged.groups[1].units, SaveBridge::get(before.globals_4f7cd0, 0, 4));
        if (!role) { return Result<Save::State>::failure(role.error()); }
        group.units.push_back(std::move(role).value());
    }
    else
    {
        SaveBridge::put(*existing, 0x23, uint32_t(x), 4);
        SaveBridge::put(*existing, 0x27, uint32_t(y), 4);
        (*existing)[0x2B] = army == 0 ? 1 : 0;
        SaveBridge::put(*existing, 8, control);
        (*existing)[0x2D] = 0;
        std::fill(existing->begin() + 0x34, existing->begin() + 0x40, 0);
        std::fill(existing->begin() + 0x124, existing->begin() + 0x13A, 0);
        constexpr std::pair<unsigned, unsigned> restore[]{ { 0xB7, 0x124 }, { 0xB9, 0x126 }, { 0xBB, 0x128 }, { 0xBD, 0x12A }, { 0xC3, 0x134 }, { 0xC5, 0x136 }, { 0xC9, 0x138 } };
        for (const auto [source, destination] : restore)
        {
            (*existing)[destination] = (*existing)[source];
            (*existing)[destination + 1] = (*existing)[source + 1];
        }
        const int ap = std::min(110, int(int16_t(unsigned((*existing)[0xBF]) | unsigned((*existing)[0xC0]) << 8)));
        SaveBridge::put(*existing, 0x12C, uint16_t(ap));
        SaveBridge::put(*existing, 0x12E, uint16_t(ap));
        SaveBridge::put(*existing, 0x30, 0x90000000, 4);
    }
    group.status[slot * 3] = 1;
    auto restored = SaveBridge::try_restore(root_, staged);
    if (!restored) { return Result<Save::State>::failure(restored.error()); }
    random = next;
    return Result<Save::State>::success(std::move(staged));
}

auto Initialization::try_sources(unsigned stage) const -> Result<std::vector<Source>>
{
    const unsigned source_stage = original_stage_id(stage);
    ResourceReader pal(pal_), bin(bin_);
    unsigned raw = pal.number(848, 4);
    if (!pal)
    {
        return Result<std::vector<Source>>::failure(pal.error());
    }
    if (raw < 11)
    {
        return Result<std::vector<Source>>::failure(make_error("Invalid source table address"));
    }
    size_t table = raw - 11;
    unsigned count = bin.number(table);
    if (!source_stage || source_stage > count)
    {
        return Result<std::vector<Source>>::failure(make_error("Invalid initialization stage"));
    }
    size_t h = table + 2 + size_t(source_stage - 1) * 30;
    if (!bin.extent(h, 30))
    {
        return Result<std::vector<Source>>::failure(bin.error());
    }
    size_t data = table + bin.number(h, 4), length = bin.number(h + 4, 4);
    if (!bin.extent(data, length))
    {
        return Result<std::vector<Source>>::failure(bin.error());
    }
    size_t offset = bin.number(h + 26, 4), units = bin.number(h + 8, 1);
    if (offset > length || units * 101 > length - offset)
    {
        return Result<std::vector<Source>>::failure(make_error("Truncated initialization roster"));
    }
    size_t begin = data + offset;
    std::vector<Source> out;
    bool enemy = false;
    for (size_t i = 0; i < units; ++i)
    {
        Source source;
        std::copy_n(bin_.begin() + begin + i * 101, 101, source.bytes.begin());
        enemy = enemy || source.behavior() == 2 || source.behavior() == 3 || source.behavior() == 8;
        source.group = enemy ? 1 : 0;
        out.push_back(source);
    }
    if (stage >= 201)
    {
        const auto path = root_ / "json" / ("stage-" + std::to_string(stage) + ".json");
        std::ifstream input(path, std::ios::binary);
        const auto json = nlohmann::json::parse(input, nullptr, false);
        if (json.is_discarded() || !json.is_object() || !json.contains("units") || !json.at("units").is_array())
        {
            return Result<std::vector<Source>>::failure(make_error("Invalid third-route initialization units: " + path.string()));
        }
        // Keep source indices aligned with OriginalEvents after omitted roles are removed.
        std::vector<Source> retained;
        size_t index = 0;
        for (const auto& source : out)
        {
            if (source.behavior() == 0)
            {
                if (source.id() != 12) { retained.push_back(source); }
                continue;
            }
            if (index >= json.at("units").size()) { continue; }
            const auto& item = json.at("units").at(index);
            const unsigned appearance = unsigned(source.bytes[16]) | (unsigned(source.bytes[17]) << 8);
            if (item.value("id", -1) == int(source.id()) && item.value("appearance", -1) == int(appearance))
            {
                retained.push_back(source);
                ++index;
            }
        }
        if (index != json.at("units").size())
        {
            return Result<std::vector<Source>>::failure(make_error("Third-route initialization source identity mismatch: " + path.string()));
        }
        out = std::move(retained);
    }
    return Result<std::vector<Source>>::success(std::move(out));
}

auto Initialization::sources(unsigned stage) const -> std::vector<Source>
{
    auto result = try_sources(stage);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::try_new_role(const Source& source, const Events::Unit& numeric, unsigned index, const PlacementMap& map,
    std::span<const Save::UnitRecord> first, std::span<const Save::UnitRecord> second) const -> Result<Save::UnitRecord>
{
    if (!index || index > 50 || numeric.id != source.id() || numeric.appearance != (unsigned(source.bytes[16]) | (unsigned(source.bytes[17]) << 8)))
    {
        return Result<Save::UnitRecord>::failure(make_error("Original source/runtime role mismatch"));
    }
    Save::UnitRecord raw{};    // 443C3E memset362, before4443B3's assignments.
    auto put = [&](size_t p, unsigned v, unsigned n = 2)
    {
        SaveBridge::put(raw, p, v, n);
    };
    put(0, index);
    put(2, numeric.id);
    put(4, numeric.appearance);
    put(6, uint16_t(source.role6()));
    put(8, source.control_class());
    put(0xA, uint16_t(source.role_a()));
    put(0xC, uint16_t(source.role_c()));
    int roleSelector;
    if (source.behavior() == 3 || source.behavior() == 9) { roleSelector = 1; }
    else if (source.behavior() == 8 || source.behavior() == 10) { roleSelector = 2; }
    else
    {
        roleSelector = 0;
    }

    raw[0x2F] = roleSelector;
    put(0x119, unsigned(source.bytes[53]) | (unsigned(source.bytes[54]) << 8));
    auto patched = SaveBridge::try_patch_unit(raw, numeric);
    if (!patched)
    {
        return Result<Save::UnitRecord>::failure(patched.error());
    }
    int x = int(int16_t(unsigned(source.bytes[23]) | (unsigned(source.bytes[24]) << 8))), y = int(int16_t(unsigned(source.bytes[25]) | (unsigned(source.bytes[26]) << 8)));
    auto position = place(x, y, map, first, second);
    put(0x23, uint32_t(position.first), 4);
    put(0x27, uint32_t(position.second), 4);
    size_t index_table = get(pal_, 808 + 8 * 64, 4) - 10;
    unsigned count = get(prg_, index_table);
    if (!numeric.appearance || numeric.appearance > count)
    {
        return Result<Save::UnitRecord>::failure(make_error("Invalid original class frame selector"));
    }
    size_t sequences = index_table + 2 + count * 8 + (numeric.appearance - 1) * 200;
    unsigned selector = uint8_t(uint32_t(position.second) >> 24);
    size_t offset = 0;
    for (unsigned i = 0; i < selector; ++i)
    {
        while (offset < 200 && prg_.at(sequences + offset))
        {
            ++offset;
        }
        if (++offset >= 200)
        {
            return Result<Save::UnitRecord>::failure(make_error("Original frame selector out of range"));
        }
    }
    raw[0x2C] = uint8_t(offset);
    unsigned first_frame = get(prg_, index_table + 2 + (numeric.appearance - 1) * 8 + 2);
    unsigned frame = prg_.at(sequences + offset);
    if (!frame)
    {
        return Result<Save::UnitRecord>::failure(make_error("Empty original initial class frame"));
    }
    size_t frame_table = get(pal_, 808 + 8 * 65, 4) - 11;
    raw[0xAC] = prg_.at(frame_table + 2 + (first_frame + frame - 1) * 7 + 6);
    return Result<Save::UnitRecord>::success(std::move(raw));
}

auto Initialization::try_insert_arrivals(const Save::State& before, const Events& expanded, unsigned turn, const PlacementMap& map, bool enemies_only, bool transition_allies) const -> Result<Save::State>
{
    if (!turn || turn > 0x7FFFFFFF)
    {
        return Result<Save::State>::failure(make_error("Invalid arrival turn"));
    }
    if (SaveBridge::get(before.globals_4f7cd0, 8, 4) != expanded.stage)
    {
        return Result<Save::State>::failure(make_error("Arrival stage mismatch"));
    }
    auto sourcesResult = try_sources(expanded.stage);
    if (!sourcesResult)
    {
        return Result<Save::State>::failure(sourcesResult.error());
    }
    auto source = std::move(sourcesResult).value();
    if (source.size() != expanded.units.size())
    {
        return Result<Save::State>::failure(make_error("Arrival requires complete source expansion"));
    }
    auto staged = before;
    for (unsigned g = enemies_only ? 1 : 0; g < (transition_allies ? 1u : 2u); ++g)
    {
        for (size_t i = 0; i < source.size(); ++i)
        {
            const auto& s = source[i];
            if (s.group != g || !s.arrives(turn) || (transition_allies && g == 0 && s.behavior() == 0))
            {
                continue;
            }
            auto& army = staged.groups[g];
            auto existing = army.units.end();
            if (s.id())
            {
                existing = std::find_if(army.units.begin(), army.units.end(), [&](const auto& r)
                    {
                        return SaveBridge::get(r, 2) == s.id();
                    });
            }
            if (existing != army.units.end())
            {
                if (!((*existing)[0x33] & 0x20))
                {
                    int x = int(int16_t(unsigned(s.bytes[23]) | (unsigned(s.bytes[24]) << 8))), y = int(int16_t(unsigned(s.bytes[25]) | (unsigned(s.bytes[26]) << 8)));
                    SaveBridge::put(*existing, 0x23, uint32_t(x), 4);
                    SaveBridge::put(*existing, 0x27, uint32_t(y), 4);
                    auto position = place(x, y, map, staged.groups[0].units, staged.groups[1].units);
                    SaveBridge::put(*existing, 0x23, uint32_t(position.first), 4);
                    SaveBridge::put(*existing, 0x27, uint32_t(position.second), 4);
                    (*existing)[0x33] |= 0x80;
                    (*existing)[0x2B] = s.bytes[50];
                    if (s.behavior() == 7)
                    {
                        (*existing)[0x33] &= 0xBF;
                    }
                }
                continue;
            }
            if (army.units.size() >= 50)
            {
                return Result<Save::State>::failure(make_error("Original arrival army capacity exceeded"));
            }
            auto roleResult = try_new_role(s, expanded.units[i], unsigned(army.units.size() + 1), map, staged.groups[0].units, staged.groups[1].units);
            if (!roleResult)
            {
                return Result<Save::State>::failure(roleResult.error());
            }
            auto raw = std::move(roleResult).value();
            auto granted = try_grant_source_consumables(army, s);
            if (!granted)
            {
                return Result<Save::State>::failure(granted.error());
            }
            army.units.push_back(raw);
        }
    }
    return Result<Save::State>::success(std::move(staged));
}

auto Initialization::try_insert_scripted_roles(const Save::State& before, const Events& expanded, size_t event, const PlacementMap& map) const -> Result<Save::State>
{
    if (expanded.type(event) == 12 && (expanded.condition(event) == 0 || expanded.condition(event) == 19 || expanded.condition(event) == 20))
    {
        return try_reset_scripted_role(before, expanded, event);
    }
    if (expanded.type(event) == 12 && (expanded.condition(event) == 22 || expanded.condition(event) == 1))
    {
        return try_mutate_scripted_control(before, expanded, event);
    }
    if (expanded.type(event) == 12 && (expanded.condition(event) == 27 || expanded.condition(event) == 28))
    {
        return try_learn_scripted_range(before, expanded, event);
    }
    if (expanded.type(event) != 6 || expanded.condition(event) != 0)
    {
        return Result<Save::State>::failure(make_error("Unsupported original scripted role operation stage " + std::to_string(expanded.stage) + " event " + std::to_string(event) + " opcode " + std::to_string(expanded.type(event)) + " condition " + std::to_string(expanded.condition(event))));
    }
    if (SaveBridge::get(before.globals_4f7cd0, 8, 4) != expanded.stage)
    {
        return Result<Save::State>::failure(make_error("Scripted role stage mismatch"));
    }
    auto sourcesResult = try_sources(expanded.stage);
    if (!sourcesResult)
    {
        return Result<Save::State>::failure(sourcesResult.error());
    }
    auto source = std::move(sourcesResult).value();
    if (source.size() != expanded.units.size())
    {
        return Result<Save::State>::failure(make_error("Scripted roles require complete source expansion"));
    }
    auto staged = before;
    for (size_t i = 0; i < source.size(); ++i)
    {
        const auto& s = source[i];
        bool match = false;
        for (unsigned offset : { 5u, 7u, 9u })
        {
            if (expanded.event_word(event, offset) > 0 && s.id() == unsigned(expanded.event_word(event, offset)))
            {
                match = true;
            }
        }
        if (!match)
        {
            continue;
        }
        auto& army = staged.groups[s.group];
        auto existing = std::find_if(army.units.begin(), army.units.end(), [&](const auto& r)
            {
                return SaveBridge::get(r, 2) == s.id();
            });
        if (existing != army.units.end())
        {
            if ((*existing)[0x33] & 0x20)
            {
                continue;
            }
            int x = int(int16_t(unsigned(s.bytes[23]) | (unsigned(s.bytes[24]) << 8))), y = int(int16_t(unsigned(s.bytes[25]) | (unsigned(s.bytes[26]) << 8)));
            auto position = place(x, y, map, staged.groups[0].units, staged.groups[1].units);
            SaveBridge::put(*existing, 0x23, uint32_t(position.first), 4);
            SaveBridge::put(*existing, 0x27, uint32_t(position.second), 4);
            (*existing)[0x33] |= 0x80;
        }
        else
        {
            if (army.units.size() >= 50)
            {
                return Result<Save::State>::failure(make_error("Original scripted army capacity exceeded"));
            }
            auto roleResult = try_new_role(s, expanded.units[i], unsigned(army.units.size() + 1), map, staged.groups[0].units, staged.groups[1].units);
            if (!roleResult)
            {
                return Result<Save::State>::failure(roleResult.error());
            }
            auto role = std::move(roleResult).value();
            auto granted = try_grant_source_consumables(army, s);
            if (!granted)
            {
                return Result<Save::State>::failure(granted.error());
            }
            army.units.push_back(role);
        }
    }
    return Result<Save::State>::success(std::move(staged));
}

auto Initialization::try_learn_scripted_range(const Save::State& before, const Events& current, size_t event) const -> Result<Save::State>
{
    int condition = current.condition(event);
    if (current.type(event) != 12 || (condition != 27 && condition != 28) || SaveBridge::get(before.globals_4f7cd0, 8, 4) != current.stage)
    {
        return Result<Save::State>::failure(make_error("Invalid original scripted learning"));
    }
    auto staged = before;
    unsigned id = unsigned(current.event_word(event, 5));
    for (auto& group : staged.groups)
    {
        for (auto& role : group.units)
        {
            if (SaveBridge::get(role, 2) == id)
            {
                if ((role[0x33] & 0x20) || !(role[0x33] & 0x80))
                {
                    return Result<Save::State>::success(std::move(staged));
                }
                unsigned school = unsigned(current.event_word(event, 7)), last = unsigned(current.event_word(event, 9));
                auto created = PromotionTransaction::create(root_);
                if (!created)
                {
                    return Result<Save::State>::failure(created.error());
                }
                auto rules = std::move(created).value();
                Events::Unit student{};
                for (unsigned g = 0; g < 11; ++g)
                {
                    student.learned[g] = SaveBridge::get(role, 0xFB + 2 * g);
                    student.learned_proficiency[g] = SaveBridge::get(role, 0xE3 + 2 * g);
                }
                if (!rules.learn_range(student, school, condition == 28 ? 1u : last, last))
                {
                    return Result<Save::State>::failure(make_error("Invalid original scripted learning range"));
                }
                for (unsigned g = 0; g < 11; ++g)
                {
                    SaveBridge::put(role, 0xFB + 2 * g, student.learned[g]);
                    SaveBridge::put(role, 0xE3 + 2 * g, student.learned_proficiency[g]);
                }
                return Result<Save::State>::success(std::move(staged));
            }
        }
    }
    return Result<Save::State>::failure(make_error("Original461110 learning role missing"));
}

auto Initialization::try_mutate_scripted_control(const Save::State& before, const Events& current, size_t event) const -> Result<Save::State>
{
    int condition = current.condition(event);
    if (current.type(event) != 12 || (condition != 22 && condition != 1) || SaveBridge::get(before.globals_4f7cd0, 8, 4) != current.stage)
    {
        return Result<Save::State>::failure(make_error("Invalid original control mutation"));
    }
    auto staged = before;
    unsigned id = unsigned(current.event_word(event, 5));
    for (auto& group : staged.groups)
    {
        for (auto& role : group.units)
        {
            if (SaveBridge::get(role, 2) == id)
            {
                if (condition >= 10 && ((role[0x33] & 0x20) || !(role[0x33] & 0x80)))
                {
                    return Result<Save::State>::success(std::move(staged));
                }
                if (condition == 22)
                {
                    SaveBridge::put(role, 8, 0);
                    SaveBridge::put(role, 10, 1);
                    SaveBridge::put(role, 12, 0);
                    role[0x33] &= 0xBF;
                }
                else
                {
                    role[0x33] = uint8_t((role[0x33] | 0x40) & 0x7F);
                }
                return Result<Save::State>::success(std::move(staged));
            }
        }
    }
    return Result<Save::State>::failure(make_error("Original461110 control mutation role missing"));
}

auto Initialization::try_reset_scripted_role(const Save::State& before, const Events& current, size_t event) const -> Result<Save::State>
{
    if (current.type(event) != 12 || (current.condition(event) != 0 && current.condition(event) != 19 && current.condition(event) != 20) || SaveBridge::get(before.globals_4f7cd0, 8, 4) != current.stage)
    {
        return Result<Save::State>::failure(make_error("Invalid original role reset transaction"));
    }
    auto staged = before;
    unsigned id = unsigned(current.event_word(event, 5));
    for (auto& group : staged.groups)
    {
        for (auto& role : group.units)
        {
            if (SaveBridge::get(role, 2) == id)
            {
                auto condition = current.condition(event);
                if (condition >= 10 && ((role[0x33] & 0x20) || !(role[0x33] & 0x80)))
                {
                    return Result<Save::State>::success(std::move(staged));
                }
                if (condition != 20)
                {
                    SaveBridge::put(role, 0x30, 0, 4);
                    SaveBridge::put(role, 8, 1);
                    SaveBridge::put(role, 2, 0);
                }
                if (condition >= 19)
                {
                    role[0x33] &= 0x7F;
                }
                return Result<Save::State>::success(std::move(staged));
            }
        }
    }
    return Result<Save::State>::failure(make_error("Original461215 role missing"));
}

auto Initialization::try_fresh(const std::filesystem::path& root, unsigned stage, unsigned difficulty,
    const std::array<uint8_t, 80>& opening_flags, const Setup& setup) const -> Result<Save::State>
{
    auto config = opening();
    const bool custom_stage = stage >= 201;
    if (!custom_stage && stage != (opening_flags[40] ? config.alternate_stage : config.primary_stage))
    {
        return Result<Save::State>::failure(make_error("Fresh stage does not match original opening flag40"));
    }
    auto created = Events::create(root, difficulty);
    if (!created)
    {
        return Result<Save::State>::failure(created.error());
    }
    auto expanded = std::move(created).value();
    expanded.flags = opening_flags;
    expanded.clear_stage_flags();
    auto loaded = expanded.try_load_stage(stage);
    if (!loaded)
    {
        return Result<Save::State>::failure(loaded.error());
    }
    Save::State raw;
    raw.flags_4f7d10 = expanded.flags;
    raw.event_states = expanded.save_event_states();
    auto put = [](auto& b, size_t p, uint32_t v, unsigned n = 4)
    {
        SaveBridge::put(b, p, v, n);
    };
    put(raw.settings_4f7cf0, 0, setup.screen_delay);
    put(raw.settings_4f7cf0, 4, setup.speed);
    put(raw.settings_4f7cf0, 8, setup.music);
    put(raw.settings_4f7cf0, 12, setup.effects);
    put(raw.settings_4f7cf0, 16, setup.speech);
    std::copy(setup.switches.begin(), setup.switches.end(), raw.settings_4f7cf0.begin() + 20);
    const unsigned source_stage = original_stage_id(stage);
    size_t table = entry(1), h = table + 2 + (source_stage - 1) * 30, data = table + get(bin_, h, 4);
    put(raw.globals_4f7cd0, 8, stage);
    put(raw.globals_4f7cd0, 12, 1);
    put(raw.globals_4f7cd0, 16, bin_.at(h + 12));
    put(raw.globals_4f7cd0, 20, bin_.at(h + 16));
    put(raw.globals_4f7cd0, 24, stage, 2);
    raw.globals_4f7cd0[28] = uint8_t(difficulty);
    put(raw.settings_4f7cf0, 27, bin_.at(h + 10));
    put(raw.stage_4fb478, 0, uint32_t(config.currency_a));
    put(raw.stage_4fb478, 4, uint32_t(config.currency_b));
    put(raw.stage_4fb478, 8, 1);
    put(raw.stage_4fb478, 12, opening_flags[40] ? 1 : 0);
    put(raw.stage_4fb478, 16, setup.display_backend);
    unsigned sections = bin_.at(h + 25);
    size_t p = data + sections * 4;
    for (unsigned i = 0; i < sections; ++i)
    {
        unsigned type = get(bin_, data + i * 4), count = get(bin_, data + i * 4 + 2);
        if (type == 5)
        {
            if (count % 4 || count > 80)
            {
                return Result<Save::State>::failure(make_error("Invalid original dynamic chains"));
            }
            put(raw.value_4f705c, 0, count / 4);
            for (size_t j = 0; j < count * 2; ++j)
            {
                raw.words_4f6fbc[j] = bin_.at(p + j);
            }
            break;
        }
        p += count * 2;
    }
    raw.groups[0] = empty_group();
    raw.groups[1] = empty_group();
    auto mapResult = try_placement_map(root, expanded.map_id);
    if (!mapResult)
    {
        return Result<Save::State>::failure(mapResult.error());
    }
    auto map = std::move(mapResult).value();
    auto arrived = try_insert_arrivals(raw, expanded, 1, map);
    if (!arrived)
    {
        return arrived;
    }
    raw = std::move(arrived).value();
    if (custom_stage)
    {
        Events male(root, difficulty);
        male.load_stage(1);
        Events female(root, difficulty);
        female.load_stage(71);
        auto male_sources = try_sources(1);
        auto female_sources = try_sources(71);
        if (!male_sources || !female_sources)
        {
            return Result<Save::State>::failure(make_error("Third-route core roster source expansion failed"));
        }
        const std::array<unsigned, 8> core_roles{ 11, 1, 14, 15, 13, 2, 3, 4 };
        auto add_core_role = [&](const Events& events, const std::vector<Source>& sources, unsigned id) -> Result<bool>
        {
            if (std::any_of(raw.groups[0].units.begin(), raw.groups[0].units.end(), [&](const auto& unit)
                    {
                        return SaveBridge::get(unit, 2) == id;
                    }))
            {
                return Result<bool>::success(true);
            }
            auto numeric = std::find_if(events.units.begin(), events.units.end(), [&](const auto& unit)
                {
                    return unit.id == id;
                });
            auto source = std::find_if(sources.begin(), sources.end(), [&](const auto& value)
                {
                    return value.id() == id;
                });
            if (numeric == events.units.end() || source == sources.end() || raw.groups[0].units.size() >= 50)
            {
                return Result<bool>::failure(make_error("Third-route core roster member missing from original source"));
            }
            auto role = try_new_role(*source, *numeric, unsigned(raw.groups[0].units.size() + 1), map, raw.groups[0].units, raw.groups[1].units);
            if (!role)
            {
                return Result<bool>::failure(role.error());
            }
            raw.groups[0].units.push_back(std::move(role).value());
            return Result<bool>::success(true);
        };
        for (unsigned id : core_roles)
        {
            const bool male_role = id == 11 || id == 14 || id == 15 || id == 13;
            auto added = add_core_role(male_role ? male : female, male_role ? male_sources.value() : female_sources.value(), id);
            if (!added) { return Result<Save::State>::failure(added.error()); }
        }
        SaveBridge::order_third_route_roster(raw);
    }
    auto described = try_describe(raw, expanded);
    if (!described)
    {
        return Result<Save::State>::failure(described.error());
    }
    return Result<Save::State>::success(std::move(raw));
}

auto Initialization::try_transition_stage(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const -> Result<Save::State>
{
    unsigned previous = SaveBridge::get(settled.globals_4f7cd0, 8, 4);
    size_t table = entry(1);
    const bool custom_previous = previous >= 201;
    const bool custom_target = deployed.stage >= 201;
    if (!previous || (!custom_previous && previous > get(bin_, table)) || !deployed.stage || (!custom_target && deployed.stage > get(bin_, table))
        || deployed.difficulty != settled.globals_4f7cd0[28] || SaveBridge::get(settled.globals_4f7cd0, 0, 4) != 0)
    {
        return Result<Save::State>::failure(make_error("Invalid original campaign transition"));
    }
    size_t prior = 0;
    unsigned direct = 0, next = 0;
    bool authorized = false;
    auto restored = SaveBridge::try_restore(root, settled);
    if (!restored)
    {
        return Result<Save::State>::failure(restored.error());
    }
    auto completed = std::move(restored).value().events;
    if (custom_previous)
    {
        authorized = Campaign(root).selected_route(completed) == deployed.stage;
    }
    else
    {
        prior = table + 2 + (previous - 1) * 30;
        direct = bin_.at(prior + 12);
        next = bin_.at(prior + 16);
        authorized = deployed.stage == direct || deployed.stage == next;
    }
    {
        Events original(root, deployed.difficulty);
        original.load_stage(previous);
        unsigned selected = Campaign(root).selected_route(completed);
        if (selected)
        {
            authorized = selected == deployed.stage;
        }
    }
    if (!custom_previous && bin_.at(prior + 21) == 3)
    {
        Events original(root, deployed.difficulty);
        original.load_stage(previous);
        authorized = Campaign::completed_story_route(original, completed, direct) == deployed.stage;
    }
    if (!authorized)
    {
        return Result<Save::State>::failure(make_error("Transition not authorized by original stage header/event"));
    }
    unsigned target_mode = custom_target ? Campaign(root).stage(deployed.stage).mode : bin_.at(table + 2 + (original_stage_id(deployed.stage) - 1) * 30 + 21);
    if (target_mode != 1 && target_mode != 3)
    {
        return Result<Save::State>::failure(make_error("Original intermediary stage requires separate scripted entry"));
    }
    Events source(root, deployed.difficulty);
    source.flags = deployed.flags;
    source.load_stage(deployed.stage);
    auto raw = settled;
    {
        // Both 40FEE9 (battle) and 410478 (story) call44A4A9 before loading arrivals.
        auto& army = raw.groups[0];
        std::vector<Save::UnitRecord> retained;
        for (size_t i = 0; i < 11; ++i)
        {
            army.status[i * 3] = 0;
        }
        for (auto role : army.units)
        {
            unsigned flags = role[0x33];
            if (SaveBridge::get(role, 8) == 1 && ((flags & 0x20) || !(flags & 0x40)))
            {
                continue;
            }
            if (flags & 0x10)
            {
                bool retainedSummon = false;
                for (unsigned slot = 0; slot < 11; ++slot)
                {
                    if ((unsigned(army.status[slot * 3 + 1]) | unsigned(army.status[slot * 3 + 2]) << 8) == SaveBridge::get(role, 4))
                    {
                        retainedSummon = true;
                        break;
                    }
                }
                if (!retainedSummon) { continue; }
                role[0x33] &= 0x73;
                role[0x2D] = 0;
                std::fill(role.begin() + 0x34, role.begin() + 0x40, uint8_t(0));
                std::fill(role.begin() + 0x124, role.begin() + 0x13A, uint8_t(0));
                for (unsigned j = 0; j < 4; ++j) { SaveBridge::put(role, 0x124 + j * 2, SaveBridge::get(role, 0xB7 + j * 2)); }
                const int ap = std::min(110, int(int16_t(SaveBridge::get(role, 0xBF))));
                SaveBridge::put(role, 0x12C, uint16_t(ap));
                SaveBridge::put(role, 0x12E, uint16_t(ap));
                for (auto [to, from] : { std::pair{ 0x134u, 0xC3u }, { 0x136u, 0xC5u }, { 0x138u, 0xC9u } })
                {
                    SaveBridge::put(role, to, SaveBridge::get(role, from));
                }
            }
            retained.push_back(role);
        }
        army.units = std::move(retained);
    }
    raw.groups[1] = empty_group();
    raw.flags_4f7d10 = deployed.flags;
    raw.event_states = deployed.save_event_states();
    SaveBridge::put(raw.globals_4f7cd0, 4, 0, 4);
    size_t h = table + 2 + (original_stage_id(deployed.stage) - 1) * 30, data = table + get(bin_, h, 4);
    SaveBridge::put(raw.globals_4f7cd0, 8, deployed.stage, 4);
    SaveBridge::put(raw.globals_4f7cd0, 12, 1, 4);
    SaveBridge::put(raw.globals_4f7cd0, 16, bin_.at(h + 12), 4);
    SaveBridge::put(raw.globals_4f7cd0, 20, bin_.at(h + 16), 4);
    SaveBridge::put(raw.globals_4f7cd0, 24, deployed.stage, 2);
    raw.globals_4f7cd0[26] = 0;
    unsigned branch = SaveBridge::get(raw.stage_4fb478, 12, 4), progress = SaveBridge::get(raw.stage_4fb478, 8, 4);
    unsigned disc = (branch == 1 && progress >= 7) || (branch == 0 && (direct == 60 || direct == 61)) ? 1 : 0;
    SaveBridge::put(raw.stage_4fb478, 16, disc, 4);
    SaveBridge::put(raw.stage_4fb478, 8, SaveBridge::get(raw.stage_4fb478, 8, 4) + 1, 4);
    SaveBridge::put(raw.settings_4f7cf0, 27, bin_.at(h + 10), 4);
    raw.value_4f705c.fill(0);
    raw.words_4f6fbc.fill(0);
    unsigned sections = bin_.at(h + 25);
    size_t p = data + sections * 4;
    for (unsigned i = 0; i < sections; ++i)
    {
        unsigned type = get(bin_, data + i * 4), count = get(bin_, data + i * 4 + 2);
        if (type == 5)
        {
            if (count % 4 || count > 80)
            {
                return Result<Save::State>::failure(make_error("Invalid original transition chains"));
            }
            SaveBridge::put(raw.value_4f705c, 0, count / 4, 4);
            for (size_t j = 0; j < count * 2; ++j)
            {
                raw.words_4f6fbc[j] = bin_.at(p + j);
            }
            break;
        }
        p += count * 2;
    }
    if (target_mode == 3)
    {
        for (size_t i = 0; i < raw.groups[0].units.size(); ++i)
        {
            auto& r = raw.groups[0].units[i];
            if (r[0x33] & 0x10)
            {
                SaveBridge::put(r, 0, unsigned(i + 1));
                continue;
            }
            unsigned original_index = SaveBridge::get(r, 0);
            if (!original_index || original_index > deployed.units.size())
            {
                return Result<Save::State>::failure(make_error("Missing retained original story slot"));
            }
            const auto& u = deployed.units[original_index - 1];
            if (SaveBridge::get(r, 2) != u.id || SaveBridge::get(r, 4) != u.appearance)
            {
                return Result<Save::State>::failure(make_error("Changed original story identity"));
            }
            auto patched = SaveBridge::try_patch_unit(r, u);
            if (!patched)
            {
                return Result<Save::State>::failure(patched.error());
            }
            SaveBridge::put(r, 0, unsigned(i + 1));
            // 410478 passes mode0:45DDA6 clears presence and resets temporary pools.
            r[0x33] &= 0x73;
            r[0x2D] = 0;
            std::fill(r.begin() + 0x34, r.begin() + 0x40, uint8_t(0));
            std::fill(r.begin() + 0x124, r.begin() + 0x13A, uint8_t(0));
            for (unsigned j = 0; j < 4; ++j)
            {
                SaveBridge::put(r, 0x124 + j * 2, SaveBridge::get(r, 0xB7 + j * 2));
            }
            unsigned ap = std::min(SaveBridge::get(r, 0xBF), 110u);
            SaveBridge::put(r, 0x12C, ap);
            SaveBridge::put(r, 0x12E, ap);
            for (auto [to, from] : { std::pair{ 0x134u, 0xC3u }, { 0x136u, 0xC5u }, { 0x138u, 0xC9u } })
            {
                SaveBridge::put(r, to, SaveBridge::get(r, from));
            }
        }
        // 40D7D4 ->427870 ->427645 creates this story turn's source actors.
        auto mapResult = try_placement_map(root, source.map_id);
        if (!mapResult)
        {
            return Result<Save::State>::failure(mapResult.error());
        }
        return try_insert_arrivals(raw, source, 1, mapResult.value());
    }
    if (deployed.units.size() < raw.groups[0].units.size())
    {
        return Result<Save::State>::failure(make_error("Missing persistent deployment roster"));
    }
    for (size_t i = 0; i < raw.groups[0].units.size(); ++i)
    {
        auto& r = raw.groups[0].units[i];
        if (r[0x33] & 0x10)
        {
            SaveBridge::put(r, 0, unsigned(i + 1));
            continue;
        }
        unsigned original_index = SaveBridge::get(r, 0);
        if (!original_index || original_index > deployed.units.size())
        {
            return Result<Save::State>::failure(make_error("Missing retained original deployment slot"));
        }
        const auto& u = deployed.units[original_index - 1];
        SaveBridge::put(r, 0, unsigned(i + 1));
        if (SaveBridge::get(r, 2) != u.id || SaveBridge::get(r, 4) != u.appearance)
        {
            return Result<Save::State>::failure(make_error("Changed original deployment identity"));
        }
        std::fill(r.begin() + 0x34, r.begin() + 0x40, uint8_t(0));
        std::fill(r.begin() + 0x124, r.begin() + 0x13A, uint8_t(0));
        auto patched = SaveBridge::try_patch_unit(r, u);
        if (!patched)
        {
            return Result<Save::State>::failure(patched.error());
        }
    }
    // 40FEE9: controllable slots were consumed by462224; nonzero-behavior
    // allied source arrivals still pass through443BB1 after enemy creation.
    auto mapResult = try_placement_map(root, source.map_id);
    if (!mapResult)
    {
        return Result<Save::State>::failure(mapResult.error());
    }
    auto map = std::move(mapResult).value();
    auto first = try_insert_arrivals(raw, source, 1, map, true);
    if (!first)
    {
        return first;
    }
    auto second = try_insert_arrivals(first.value(), source, 1, map, false, true);
    if (!second)
    {
        return second;
    }
    auto arrivals = std::move(second).value();
    auto described = try_describe(arrivals, deployed);
    if (!described)
    {
        return Result<Save::State>::failure(described.error());
    }
    return Result<Save::State>::success(std::move(arrivals));
}

auto Initialization::try_transition_stage2(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const -> Result<Save::State>
{
    if (SaveBridge::get(settled.globals_4f7cd0, 8, 4) != 1 || deployed.stage != 2)
    {
        return Result<Save::State>::failure(make_error("Stage2 compatibility entry requires1->2"));
    }
    return try_transition_stage(root, settled, deployed);
}

auto Initialization::try_grant_source_consumables(Save::Group& group, const Source& source) const -> Result<bool>
{
    if (group.inventory.size() != Save::item_count(items_))
    {
        return Result<bool>::failure(make_error("Complete original inventory required"));
    }
    auto staged = group.inventory;
    for (unsigned i = 0; i < 6; ++i)
    {
        unsigned item = source.bytes[63 + i * 2] | (unsigned(source.bytes[64 + i * 2]) << 8);
        if (!item)
        {
            continue;
        }
        if (item > staged.size())
        {
            return Result<bool>::failure(make_error("Invalid original source item"));
        }
        int type = int(int8_t(items_.at(2 + (item - 1) * 38 + 14)));
        if (type < 1 || type > 12)
        {
            return Result<bool>::failure(make_error("Invalid original source item type"));
        }
        if (type == 12)
        {
            ++staged[item - 1];
        }
    }
    group.inventory.swap(staged);
    return Result<bool>::success(true);
}

auto Initialization::grant_source_consumables(Save::Group& group, const Source& source) const -> void
{
    auto result = try_grant_source_consumables(group, source);
    if (!result)
    {
        fatal(result.error());
    }
}

auto Initialization::new_role(const Source& source, const Events::Unit& numeric, unsigned index, const PlacementMap& map,
    std::span<const Save::UnitRecord> first, std::span<const Save::UnitRecord> second) const -> Save::UnitRecord
{
    auto result = try_new_role(source, numeric, index, map, first, second);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::insert_arrivals(const Save::State& before, const Events& expanded, unsigned turn, const PlacementMap& map, bool enemies_only, bool transition_allies) const -> Save::State
{
    auto result = try_insert_arrivals(before, expanded, turn, map, enemies_only, transition_allies);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::insert_scripted_roles(const Save::State& before, const Events& expanded, size_t event, const PlacementMap& map) const -> Save::State
{
    auto result = try_insert_scripted_roles(before, expanded, event, map);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::learn_scripted_range(const Save::State& before, const Events& current, size_t event) const -> Save::State
{
    auto result = try_learn_scripted_range(before, current, event);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::mutate_scripted_control(const Save::State& before, const Events& current, size_t event) const -> Save::State
{
    auto result = try_mutate_scripted_control(before, current, event);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::reset_scripted_role(const Save::State& before, const Events& current, size_t event) const -> Save::State
{
    auto result = try_reset_scripted_role(before, current, event);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::fresh(const std::filesystem::path& root, unsigned stage, unsigned difficulty,
    const std::array<uint8_t, 80>& opening_flags, const Setup& setup) const -> Save::State
{
    auto result = try_fresh(root, stage, difficulty, opening_flags, setup);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::transition_stage(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const -> Save::State
{
    auto result = try_transition_stage(root, settled, deployed);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Initialization::transition_stage2(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const -> Save::State
{
    auto result = try_transition_stage2(root, settled, deployed);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}
}    //namespace s2ne
