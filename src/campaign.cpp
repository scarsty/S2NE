#include "campaign.h"

namespace s2ne
{
auto Campaign::table() const -> size_t { return dword(pal_, 848) - 11; }

auto Campaign::try_header(unsigned stage) const -> s2ne::Result<size_t>
{
    if (!stage || stage > word(bin_, table()))
    {
        return s2ne::Result<size_t>::failure(make_error("Invalid campaign stage"));
    }
    return s2ne::Result<size_t>::success(table() + 2 + (stage - 1) * 30);
}

auto Campaign::header(unsigned stage) const -> size_t
{
    auto result = try_header(stage);
    return std::move(result).value();
}

auto Campaign::pixel(std::vector<uint8_t>& image, int x, int y, unsigned color) const -> void
{
    if (x < 0 || y < 0 || x >= 640 || y >= 480)
    {
        return;
    }
    size_t p = (size_t(y) * 640 + x) * 4;
    for (unsigned k = 0; k < 3; ++k)
    {
        image.at(p + k) = uint8_t(unsigned(pal_.at(color * 3 + k)) * 255 / 63);
    }
    image.at(p + 3) = 255;
}

auto Campaign::create(const std::filesystem::path& root) -> Result<Campaign>
{
    Campaign result;
    result.root_ = root;
    auto font = Menu::create(root);
    if (!font)
    {
        return Result<Campaign>::failure(font.error());
    }
    result.font_ = std::move(font).value();
    for (auto pair : { std::pair{ &result.bin_, "SUPERSP.BIN" }, std::pair{ &result.pal_, "SUPERSP.PAL" }, std::pair{ &result.cel_, "SUPERSP.CEL" }, std::pair{ &result.prg_, "SUPERSP.PRG" } })
    {
        auto bytes = read_shared_resource(root / pair.second);
        if (!bytes)
        {
            return Result<Campaign>::failure(bytes.error());
        }
        *pair.first = std::move(bytes).value();
    }
    auto items = read_resource(root / "ITEMSP.BIN");
    if (!items) { return Result<Campaign>::failure(items.error()); }
    result.items_ = std::move(items).value();
    ResourceReader pal(result.pal_), bin(result.bin_);
    unsigned raw = pal.number(848, 4);
    if (!pal)
    {
        return Result<Campaign>::failure(pal.error());
    }
    if (raw < 11)
    {
        return Result<Campaign>::failure(make_error("Invalid campaign table address"));
    }
    unsigned count = bin.number(raw - 11);
    if (!bin)
    {
        return Result<Campaign>::failure(bin.error());
    }
    for (unsigned id = 1; id <= count; ++id)
    {
        auto stage = result.try_stage(id);
        if (!stage)
        {
            return Result<Campaign>::failure(stage.error());
        }
    }
    return Result<Campaign>::success(std::move(result));
}

Campaign::Campaign(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto Campaign::try_stage(unsigned id) const -> Result<Stage>
{
    if (id >= 201)
    {
        const auto json_path = root_ / "json" / ("stage-" + std::to_string(id) + ".json");
        if (!std::filesystem::exists(json_path))
        {
            return Result<Stage>::failure(make_error("Missing third-route stage JSON: " + json_path.string()));
        }
        std::ifstream input(json_path, std::ios::binary);
        if (!input)
        {
            return Result<Stage>::failure(make_error("Cannot open third-route stage JSON: " + json_path.string()));
        }
        std::string text(std::istreambuf_iterator<char>(input), {});
        auto json = nlohmann::json::parse(text, nullptr, false);
        if (json.is_discarded() || !json.is_object() || !json.contains("route") || !json.at("route").is_object())
        {
            return Result<Stage>::failure(make_error("Invalid third-route stage JSON: " + json_path.string()));
        }
        const auto& route = json.at("route");
        if (!route.contains("next_stage") || !route.at("next_stage").is_number_unsigned() || !route.contains("mode")
            || !route.at("mode").is_number_unsigned())
        {
            return Result<Stage>::failure(make_error("Incomplete third-route stage route: " + json_path.string()));
        }
        auto original = try_stage(original_stage_id(id));
        if (!original) { return original; }
        Stage stage = std::move(original).value();
        stage.id = id;
        stage.next = route.at("next_stage").get<unsigned>();
        stage.mode = route.at("mode").get<unsigned>();
        return Result<Stage>::success(std::move(stage));
    }
    const unsigned source_id = original_stage_id(id);
    ResourceReader pal(pal_), bin(bin_);
    unsigned raw = pal.number(848, 4);
    if (!pal)
    {
        return Result<Stage>::failure(pal.error());
    }
    if (raw < 11)
    {
        return Result<Stage>::failure(make_error("Invalid campaign stage table"));
    }
    size_t table = raw - 11;
    unsigned count = bin.number(table);
    if (!source_id || source_id > count)
    {
        return Result<Stage>::failure(make_error("Invalid campaign stage"));
    }
    size_t h = table + 2 + size_t(source_id - 1) * 30;
    if (!bin.extent(h, 30))
    {
        return Result<Stage>::failure(bin.error());
    }
    size_t data = table + bin.number(h, 4), length = bin.number(h + 4, 4);
    if (!bin.extent(data, length))
    {
        return Result<Stage>::failure(bin.error());
    }
    size_t end = data + length;
    Stage stage;
    stage.id = id;
    stage.next = bin.number(h + 12, 1);
    stage.mode = bin.number(h + 21, 1);
    const auto json_path = root_ / "json" / ("stage-" + std::to_string(id) + ".json");
    if (std::filesystem::exists(json_path))
    {
        std::ifstream input(json_path, std::ios::binary);
        if (!input)
        {
            return Result<Stage>::failure(make_error("Cannot open stage JSON route: " + json_path.string()));
        }
        std::string text(std::istreambuf_iterator<char>(input), {});
        auto json = nlohmann::json::parse(text, nullptr, false);
        if (json.is_discarded() || !json.is_object() || !json.contains("route") || !json.at("route").is_object())
        {
            return Result<Stage>::failure(make_error("Invalid stage JSON route: " + json_path.string()));
        }
        const auto& route = json.at("route");
        stage.next = route.value("next_stage", stage.next);
        stage.mode = route.value("mode", stage.mode);
    }
    unsigned sections = bin.number(h + 25, 1);
    size_t values = data + sections * 4;
    if (values > end)
    {
        return Result<Stage>::failure(make_error("Truncated campaign directory"));
    }
    for (unsigned i = 0; i < sections; ++i)
    {
        unsigned type = bin.number(data + i * 4), n = bin.number(data + i * 4 + 2);
        if (size_t(n) * 2 > end - values)
        {
            return Result<Stage>::failure(make_error("Truncated campaign reward"));
        }
        if (type >= 1 && type <= 4)
        {
            Reward reward;
            reward.type = type;
            for (unsigned j = 0; j < n; ++j)
            {
                reward.values.push_back(int(int16_t(bin.number(values + j * 2))));
            }
            stage.rewards.push_back(std::move(reward));
        }
        values += size_t(n) * 2;
    }
    size_t section = data + bin.number(h + 26, 4) + bin.number(h + 8, 1) * 101;
    if (section > end || end - section < 14)
    {
        return Result<Stage>::failure(make_error("Invalid campaign objectives"));
    }
    size_t total = 0;
    for (unsigned i : { 0u, 2u, 4u, 6u })
    {
        total += bin.number(section + i);
    }
    for (unsigned i : { 8u, 10u, 12u })
    {
        total += size_t(bin.number(section + i)) * 2;
    }
    if (total > end - section - 14)
    {
        return Result<Stage>::failure(make_error("Campaign conditions outside stage"));
    }
    size_t p = section + 14;
    for (unsigned i = 0; i < bin.number(section); ++i)
    {
        uint8_t c = bin_[p + i];
        if (c)
        {
            stage.name.push_back(c);
        }
    }
    p += bin.number(section) + bin.number(section + 2) + bin.number(section + 4);
    for (unsigned k = 0; k < 2; ++k)
    {
        for (unsigned j = 0; j < bin.number(section + 8 + k * 2); ++j)
        {
            stage.victory[k].push_back(int(int16_t(bin.number(p))));
            p += 2;
        }
    }
    p += bin.number(section + 6);
    for (unsigned j = 0; j < bin.number(section + 12); ++j)
    {
        stage.defeat.push_back(int(int16_t(bin.number(p))));
        p += 2;
    }
    if (!bin)
    {
        return Result<Stage>::failure(bin.error());
    }
    return Result<Stage>::success(std::move(stage));
}

auto Campaign::stage(unsigned id) const -> Stage
{
    auto result = try_stage(id);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto Campaign::try_story_animation_group(const Events& events, size_t begin) -> s2ne::Result<StoryAnimationGroup>
{
    if (begin >= events.event_count())
    {
        return s2ne::Result<StoryAnimationGroup>::failure(make_error("Invalid grouped animation event index"));
    }
    unsigned condition = events.condition(begin);
    if (events.type(begin) != 15 || (condition != 0 && condition != 3))
    {
        return s2ne::Result<StoryAnimationGroup>::failure(make_error("Not an original grouped animation event"));
    }
    int capacity = events.event_word(begin, 5), flag = events.event_word(begin, 3);
    if (capacity <= 0 || flag < 1 || flag > 80 || !events.flags[size_t(flag - 1)])
    {
        return s2ne::Result<StoryAnimationGroup>::failure(make_error("Original grouped animation trigger unavailable"));
    }
    StoryAnimationGroup group;
    group.hostile = condition == 3;
    group.begin = begin;
    group.end = events.event_count();
    for (size_t i = begin + 1; i < events.event_count(); ++i)
    {
        if (events.type(i) != 15 || events.event_word(i, 3) != flag)
        {
            continue;
        }
        if (events.condition(i) == condition + 2)
        {
            group.end = i;
            break;
        }
        if (events.condition(i) != condition + 1)
        {
            continue;
        }
        if (group.roles.size() >= size_t(capacity))
        {
            return s2ne::Result<StoryAnimationGroup>::failure(make_error("Original grouped animation capacity exceeded"));
        }
        int id = events.event_word(i, 5);
        if (id > 0)
        {
            group.roles.push_back({ unsigned(id), events.event_word(i, 7), events.event_word(i, 9) * 4, i });
        }
    }
    if (group.end == events.event_count())
    {
        return s2ne::Result<StoryAnimationGroup>::failure(make_error("Original grouped animation terminator missing"));
    }
    return s2ne::Result<StoryAnimationGroup>::success(group);
}

auto Campaign::story_animation_group(const Events& events, size_t begin) -> StoryAnimationGroup
{
    auto result = try_story_animation_group(events, begin);
    return std::move(result).value();
}

auto Campaign::try_story_movement_group(const Events& events, size_t event) -> s2ne::Result<StoryMovementGroup>
{
    if (event >= events.event_count())
    {
        return s2ne::Result<StoryMovementGroup>::failure(make_error("Invalid grouped movement event index"));
    }
    int condition = events.condition(event), flag = events.event_word(event, 3), capacity = events.event_word(event, 5);
    if (events.type(event) != 16 || (condition != 0 && condition != 4) || capacity <= 0)
    {
        return s2ne::Result<StoryMovementGroup>::failure(make_error("Invalid original grouped movement header"));
    }
    StoryMovementGroup group{ event, events.event_count(), condition == 4, unsigned(events.event_word(event, 7)), {} };
    for (size_t i = event + 1; i < events.event_count(); ++i)
    {
        if (events.type(i) != 16 || events.event_word(i, 3) != flag)
        {
            continue;
        }
        if (events.condition(i) == condition + 3)
        {
            group.end = i;
            break;
        }
        if (events.condition(i) == condition + 2)
        {
            return s2ne::Result<StoryMovementGroup>::failure(make_error("Original grouped movement role-relative operand requires461808"));
        }
        if (events.condition(i) != condition + 1)
        {
            continue;
        }
        int id = events.event_word(i, 5);
        if (id <= 0)
        {
            continue;
        }
        if (group.roles.size() >= size_t(capacity))
        {
            return s2ne::Result<StoryMovementGroup>::failure(make_error("Original grouped movement capacity exceeded"));
        }
        group.roles.push_back({ unsigned(id), events.event_word(i, 7), events.event_word(i, 9), i });
    }
    // 422074 also ends the scan at event_count; a same-flag terminator is optional.
    return s2ne::Result<StoryMovementGroup>::success(group);
}

auto Campaign::story_movement_group(const Events& events, size_t event) -> StoryMovementGroup
{
    auto result = try_story_movement_group(events, event);
    return std::move(result).value();
}

auto Campaign::try_completed_story_route(const Events& source, const Events& completed, unsigned header_next) -> s2ne::Result<unsigned>
{
    if (source.stage != completed.stage || source.event_count() != completed.event_count())
    {
        return s2ne::Result<unsigned>::failure(make_error("Mismatched original story event state"));
    }
    unsigned target = 0;
    for (size_t i = 0; i < source.event_count(); ++i)
    {
        if (source.type(i) != 9 || source.condition(i) != 6 || completed.type(i) != 0)
        {
            continue;
        }
        int flag = source.event_word(i, 3), route = source.event_word(i, 5);
        if (flag >= 1 && flag <= 80 && completed.flags[size_t(flag - 1)] && route > 0)
        {
            // 41C397/41C3A0 scan records in order; each successful 420DC3
            // assignment replaces 4F7CE0, including later flag-chain overrides.
            target = unsigned(route);
        }
    }
    if (!target && header_next)
    {
        bool exhausted = true;
        for (size_t i = 0; i < source.event_count(); ++i)
        {
            if (source.type(i) && completed.type(i))
            {
                // Group operands/terminators with no trigger are not standalone pending events.
                if (source.type(i) == 16 && (source.condition(i) == 3 || source.condition(i) == 7) && source.event_word(i, 3) == 0)
                {
                    continue;
                }
                // 41B9A3 scans false predicates without consuming their records.
                if (auto predicate = original_role_predicate(completed, i); predicate && !*predicate)
                {
                    continue;
                }
                exhausted = false;
                break;
            }
        }
        if (exhausted)
        {
            target = header_next;
        }
    }
    return s2ne::Result<unsigned>::success(target);
}

auto Campaign::completed_story_route(const Events& source, const Events& completed, unsigned header_next) -> unsigned
{
    auto result = try_completed_story_route(source, completed, header_next);
    return std::move(result).value();
}

auto Campaign::selected_route(const Events& completed) const -> unsigned
{
    if (is_custom_stage(completed.stage) && (!completed.script_route || *completed.script_route < 201)) { return stage(completed.stage).next; }
    // 420DC3写入4F7CE0后，后续旗标可能已被消耗；优先使用事务保存的路由。
    if (completed.script_route && *completed.script_route > 0) { return unsigned(*completed.script_route); }
    Events source = completed;
    source.load_stage(completed.stage);
    unsigned route = completed_story_route(source, completed);
    return route ? route : stage(completed.stage).next;
}

auto Campaign::try_animation_sequence_length(unsigned appearance, unsigned sequence) const -> s2ne::Result<unsigned>
{
    size_t index = dword(pal_, 808 + 8 * 64) - 10;
    unsigned count = word(prg_, index);
    if (!appearance || appearance > count || sequence >= 30)
    {
        return s2ne::Result<unsigned>::failure(make_error("Unsupported original grouped appearance sequence"));
    }
    size_t cursor = index + 2 + count * 8 + (appearance - 1) * 200, end = cursor + 200;
    for (unsigned i = 0; i < sequence; ++i)
    {
        while (cursor < end && prg_.at(cursor))
        {
            ++cursor;
        }
        if (++cursor >= end)
        {
            return s2ne::Result<unsigned>::failure(make_error("Missing original grouped sequence"));
        }
    }
    unsigned length = 0;
    while (cursor + length < end && prg_.at(cursor + length))
    {
        ++length;
    }
    if (cursor + length == end)
    {
        return s2ne::Result<unsigned>::failure(make_error("Unterminated original grouped sequence"));
    }
    return s2ne::Result<unsigned>::success(length);
}

auto Campaign::animation_sequence_length(unsigned appearance, unsigned sequence) const -> unsigned
{
    auto result = try_animation_sequence_length(appearance, sequence);
    return std::move(result).value();
}

auto Campaign::try_protagonist_at_exit(const Events& events) const -> s2ne::Result<bool>
{
    unsigned id = events.flags[40] ? 1 : 11;
    auto role = std::find_if(events.units.begin(), events.units.end(), [&](const auto& u)
        {
            return s2ne::Result<bool>::success(!u.hostile() && u.id == id && living(u));
        });
    if (role == events.units.end())
    {
        return s2ne::Result<bool>::success(false);
    }
    size_t maps = dword(pal_, 808 + 8 * (64 + 6 - 1)) - 15;
    size_t map = maps + dword(prg_, maps + 2 + (events.map_id - 1) * 8);
    int width = int(word(prg_, map)), height = int(word(prg_, map + 2)) * 2;
    if (role->x < 0 || role->y < 0 || role->x >= width - (role->y & 1) || role->y >= height)
    {
        return s2ne::Result<bool>::success(false);
    }
    size_t cell = map + 28 + (size_t(role->y) * width + role->x) * 5;
    unsigned object = word(prg_, cell + 3);
    if (!object)
    {
        return s2ne::Result<bool>::success(false);
    }
    size_t attrs = dword(pal_, 808 + 8 * (64 + 7 - 1)) - 16;
    if (object > word(prg_, attrs + 2))
    {
        return s2ne::Result<bool>::failure(make_error("Invalid original exit object"));
    }
    size_t objects = attrs + 6 + word(prg_, attrs) * 2;
    return s2ne::Result<bool>::success((word(prg_, objects + (object - 1) * 2) >> 8) == 6);
}

auto Campaign::protagonist_at_exit(const Events& events) const -> bool
{
    auto result = try_protagonist_at_exit(events);
    if (!result)
    {
        report_error(result.error());
        return false;
    }
    return result.value();
}

auto Campaign::living(const Events::Unit& u) -> bool { return (u.status_flags & 0x80) && !(u.status_flags & 0x20); }

auto Campaign::absent(std::span<const Events::Unit> roster, int selector) -> bool
{
    if (selector > 0)
    {
        for (const auto& u : roster)
        {
            if (int(u.id) == selector && (u.status_flags & 0x80) && (u.status_flags & 0x20))
            {
                return true;
            }
        }
        return false;
    }
    for (const auto& u : roster)
    {
        if (selector == -1 && u.behavior != 1)
        {
            continue;
        }
        if (selector == -2 && u.runtime_role_selector != 1)
        {
            continue;
        }
        if (selector == -3 && u.runtime_role_selector == 1)
        {
            continue;
        }
        if (living(u))
        {
            return false;
        }
    }
    return true;
}

auto Campaign::evaluate(const Stage& s, std::span<const Events::Unit> allies,
    std::span<const Events::Unit> enemies, const Runtime& rt) -> Decision
{
    if (!rt.idle)
    {
        return {};
    }
    // Demo mode uses a different roster-only rule (4383E0/4384FE).
    if (rt.mode == 1)
    {
        if (absent(allies, 0))
        {
            return { Outcome::defeat, 0 };
        }
        if (absent(enemies, 0))
        {
            return { Outcome::victory, 0 };
        }
        return {};
    }
    if (rt.turn > 99 || rt.forced_defeat)
    {
        return { Outcome::defeat, 0 };
    }
    for (int c : s.defeat)
    {
        if ((c == 0 && absent(allies, -1)) || (c >= 1 && c <= 43 && absent(allies, c)) || (c > 100 && c <= 190 && rt.turn >= unsigned(c - 100)))
        {
            return { Outcome::defeat, 0 };
        }
    }
    if (rt.forced_victory)
    {
        return { Outcome::victory, 0 };
    }
    for (unsigned k = 0; k < 2; ++k)
    {
        if (s.victory[k].empty())
        {
            continue;
        }
        bool all = true;
        for (int c : s.victory[k])
        {
            bool satisfied;
            if (c == 0) { satisfied = absent(enemies, 0); }
            else if (c == 33) { satisfied = absent(enemies, -2); }
            else if (c == 34) { satisfied = absent(enemies, -3); }
            else
            {
                satisfied = ((c >= 1 && c <= 20) || c == 31 || c == 32) && rt.special_condition && rt.special_condition(c);
            }

            if (!satisfied)
            {
                all = false;
                break;
            }
        }
        if (all)
        {
            return { Outcome::victory, k + 1 };
        }
    }
    return {};
}

auto Campaign::victory_events(const Events& events, unsigned group) -> std::vector<size_t>
{
    std::vector<size_t> result;
    for (size_t i = 0; i < events.event_count(); ++i)
    {
        if (events.type(i) == 2 && (events.condition(i) == 0 || events.condition(i) == group))
        {
            result.push_back(i);
        }
    }
    return result;
}

auto Campaign::try_plan(const Stage& s, Decision d, std::span<const Events::Unit> allies) -> s2ne::Result<Settlement>
{
    if (d.outcome != Outcome::victory)
    {
        return s2ne::Result<Settlement>::failure(make_error("Cannot settle a non-victory"));
    }
    Settlement p;
    p.stage = s.id;
    p.next = s.next;
    p.group = d.group;
    p.rewards = s.rewards;
    for (size_t i = 0; i < allies.size(); ++i)
    {
        if (living(allies[i]))
        {
            p.survivor_indices.push_back(i);
        }
    }
    return s2ne::Result<Settlement>::success(p);
}

auto Campaign::plan(const Stage& s, Decision d, std::span<const Events::Unit> allies) -> Settlement
{
    auto result = try_plan(s, d, allies);
    return std::move(result).value();
}

auto Campaign::save_request(SavePoint point, unsigned original_mode, unsigned turn) -> std::optional<SaveRequest>
{
    if (point == SavePoint::rotate_previous)
    {
        return SaveRequest{ point, 53, false };
    }
    if (original_mode == 1 || (point == SavePoint::before_stage_story && turn != 1))
    {
        return std::nullopt;
    }
    return SaveRequest{ point, point == SavePoint::before_stage_story ? 51u : 52u, true };
}

auto Campaign::autosave_slot(unsigned original_mode, unsigned turn, bool before_story) -> std::optional<unsigned>
{
    if (original_mode == 1 || (before_story && turn != 1))
    {
        return std::nullopt;
    }
    return before_story ? 51u : 52u;
}

auto Campaign::victory_experience(unsigned xp) -> unsigned { return uint16_t(xp + 30u); }

auto Campaign::try_render_rewards_validated(std::vector<uint8_t>& image, const Stage& s,
    std::span<const Events::Unit> roster) const -> s2ne::Result<bool>
{
    if (image.size() != 640u * 480 * 4)
    {
        return s2ne::Result<bool>::failure(make_error("Invalid campaign framebuffer"));
    }
    for (const auto& r : s.rewards)
    {
        if (r.type != 1 && r.type != 3 && r.type != 4)
        {
            return s2ne::Result<bool>::success(false);
        }
        if (r.type == 4)
        {
            for (int id : r.values)
            {
                if (id < 1 || unsigned(id) > word(items_, 0))
                {
                    return s2ne::Result<bool>::success(false);
                }
            }
        }
    }
    for (const auto& r : s.rewards)
    {
        if (r.type == 1)
        {
            for (int id : r.values)
            {
                if (std::none_of(roster.begin(), roster.end(), [&](const auto& u)
                        {
                            return int(u.id) == id;
                        }))
                {
                    return s2ne::Result<bool>::success(false);
                }
            }
        }
    }
    size_t g = dword(pal_, 816) - 57, p = g + dword(cel_, g + 2 + (207 - 142) * 8);
    int left = 320 - int(word(cel_, p) + 1) / 2, top = 240 - int(word(cel_, p + 2) + 1) / 2;
    auto panel_result = draw_original_cel(image, cel_, pal_, 207, left, top, ZeroRun::transparent);
    if (!panel_result)
    {
        return panel_result;
    }
    auto draw = [&](unsigned id, int x, int y, unsigned c)
    {
        font_.draw_original_large(image, font_.text_bytes(id), x, y, c);
    };
    auto title = font_.text_bytes(34);
    font_.draw_original_large(image, title, left + 310 - int(title.size()) * 6, top + 17, 57);
    draw(48, left + 90, top + 57, 54);
    font_.draw_original_large(image, s.name, left + 213, top + 57, 54);
    int y = top + 108;
    for (const auto& r : s.rewards)
    {
        if (r.type == 1)
        {
            draw(35, left + 90, y, 119);
            for (size_t j = 0; j < r.values.size(); ++j)
            {
                auto it = std::find_if(roster.begin(), roster.end(), [&](const auto& u)
                    {
                        return int(u.id) == r.values[j];
                    });
                font_.draw_original_large(image, it->name, left + 205 + int(j % 2) * 163, y + int(j / 2) * 25, 119);
            }
            y += int((r.values.size() + 1) / 2) * 25;
        }
        if (r.type == 3 && !r.values.empty())
        {
            draw(37, left + 90, top + 330, 119);
            auto value = std::to_string(r.values[0]);
            font_.draw_original_large(image, { value.begin(), value.end() }, left + 220, top + 330, 119);
        }
        if (r.type == 4)
        {
            draw(38, left + 90, y, 119);
            for (size_t j = 0; j < r.values.size(); ++j)
            {
                int x = left + 198 + int(j % 2) * 168, yy = y + int(j / 2) * 30;
                auto icon_result = draw_original_cel(image, cel_, pal_, unsigned(r.values[j]) + 492, x, yy, ZeroRun::transparent);
                if (!icon_result)
                {
                    return icon_result;
                }
                size_t name = 2 + size_t(r.values[j] - 1) * 38 + 2;
                std::vector<uint8_t> text;
                for (size_t k = 0; k < 12 && items_.at(name + k); ++k)
                {
                    text.push_back(items_.at(name + k));
                }
                font_.draw_original_large(image, text, left + 236 + int(j % 2) * 168, yy, 119);
            }
            y += int((r.values.size() + 1) / 2) * 30;
        }
    }
    return s2ne::Result<bool>::success(true);
}

auto Campaign::try_render_rewards(std::vector<uint8_t>& image, const Stage& s,
    std::span<const Events::Unit> roster) const -> s2ne::Result<bool>
{
    auto output = image;
    auto result = try_render_rewards_validated(output, s, roster);
    if (result)
    {
        image.swap(output);
    }
    return result;
}

auto Campaign::render_rewards(std::vector<uint8_t>& image, const Stage& s,
    std::span<const Events::Unit> roster) const -> bool
{
    auto result = try_render_rewards(image, s, roster);
    return std::move(result).value();
}
}    //namespace s2ne
