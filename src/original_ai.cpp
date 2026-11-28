#include "original_ai.h"

namespace s2ne
{
auto multiply(int a, int b) -> int { return std::bit_cast<int32_t>(uint32_t(a) * uint32_t(b)); }

auto subtract(int a, int b) -> int { return std::bit_cast<int32_t>(uint32_t(a) - uint32_t(b)); }

auto physical_preview_value(int chance, int damage, bool status_bonus) -> int { return multiply(chance + (status_bonus ? 10 : 0), damage); }

auto score_melee(const MeleePreview& p, Weights w) -> std::optional<Score>
{
    bool priority = p.target_kind == 1 || p.target_kind == 11 || (p.target_flags2f & 3) != 0;
    int value = p.attack_value;
    if (priority)
    {
        value = multiply(value, w.priority);
    }
    bool kill = p.attack_damage > p.target_hp && p.attack_chance > 50;
    if (kill)
    {
        value = multiply(value, w.kill);
    }
    value = subtract(value, p.counter_value);
    if (p.counter_damage > p.attacker_hp && p.counter_chance > 50 && !p.attacker_status38_02)
    {
        return std::nullopt;
    }
    if (value <= 0)
    {
        return std::nullopt;
    }
    return Score{ priority && kill ? 1 : 2, value, kill ? 1 : 0 };
}

auto improves_melee(Score candidate, Score current) -> bool
{
    if (candidate.rank != 1 && candidate.rank != 2)
    {
        return false;
    }
    if (current.rank == 0)
    {
        return true;
    }
    if (current.rank == 3 && candidate.rank == 2)
    {
        return candidate.kills > 0 || candidate.value > current.value;
    }
    if (candidate.rank == 1)
    {
        return current.rank != 1 || candidate.value > current.value;
    }
    if (current.rank != 2)
    {
        return false;
    }
    if (candidate.kills > 0)
    {
        return candidate.kills > current.kills || (candidate.kills == current.kills && candidate.value > current.value);
    }
    return current.kills == 0 && candidate.value > current.value;
}

auto improves_skill(Score candidate, Score current, int threshold) -> bool
{
    if (candidate.rank <= 0) { return false; }
    if (!current.rank) { return true; }
    if (candidate.rank == 1) { return current.rank != 1 || candidate.value > current.value; }
    if (candidate.rank == 2)
    {
        if (candidate.kills > 0)
        {
            if (current.rank > 2) { return true; }
            return current.rank == 2 && (candidate.kills > current.kills || (candidate.kills == current.kills && candidate.value > current.value));
        }
        if (current.rank == 2) { return !current.kills && candidate.value > current.value; }
        if (current.rank == 3) { return candidate.value > current.value; }
        return current.rank > 3 && (candidate.value > threshold || (candidate.value < threshold && candidate.value > current.value));
    }
    if (candidate.rank == 3)
    {
        if (current.rank == 2) { return !current.kills && candidate.value > current.value; }
        if (current.rank == 3) { return candidate.value > current.value; }
        return current.rank > 3 && (candidate.value > threshold / 2 || (candidate.value < threshold / 2 && candidate.value > current.value));
    }
    if (current.rank == 2) { return current.value < threshold && candidate.value > current.value; }
    if (current.rank == 3) { return current.value < threshold / 2 && candidate.value > current.value; }
    return current.rank > 3 && candidate.value > current.value;
}

auto scan_melee_position(std::span<const Candidate> candidates, Weights weights, Selection& selected, int skill_threshold) -> bool
{
    for (const auto& candidate : candidates)
    {
        auto score = score_melee(candidate.preview, weights);
        if (score && improves_skill(*score, selected.score, skill_threshold))
        {
            selected.score = *score;
            selected.candidate = candidate;
        }
    }
    return selected.score.rank == 1;
}

auto melee_route_candidate(int signed_record7, unsigned category12, bool movement_used) -> bool
{
    return !movement_used && signed_record7 != 0 && category12 >= 1;
}

auto skill_route_allowed(const Score& score, int movement_mode) -> bool
{
    return (score.rank < 30 || movement_mode == 1) && (score.rank <= 0 || score.value != 0);
}

auto movement_mode(bool helper40CAFE, bool helper40C956) -> int
{
    if (helper40CAFE) { return 0; }
    else if (helper40C956) { return 1; }
    else
    {
        return 3;
    }
}

auto original_distance(Position from, Position to) -> int
{
    for (int radius = 1; radius < 80; ++radius)
    {
        from.x -= (from.y & 1) ? 0 : 1;
        --from.y;
        if ((to.y == from.y || to.y == from.y + 2 * radius) && to.x >= from.x && to.x <= from.x + radius)
        {
            return radius;
        }
        if ((to.x == from.x || to.x == from.x + radius) && to.y > from.y && to.y < from.y + 2 * radius && ((to.y - from.y) & 1) == 0)
        {
            return radius;
        }
    }
    return 80;
}

auto OrderedRoutes::candidates() const -> std::vector<RouteRecord>
{
    std::vector<RouteRecord> result;
    for (std::size_t i = 1; i < order.size(); ++i)
    {
        result.push_back(records.at(order[i]).route);
    }
    return result;
}

auto original_target(std::span<const MovementTarget> units, Position origin, int mode, int minimum_value) -> const MovementTarget*
{
    const MovementTarget* best = nullptr;
    int distance = 0;
    for (const auto& unit : units)
    {
        if (!unit.runtime_ad_nonzero || (unit.status33 & 0x20) || !(unit.status33 & 0x80) || unit.position == origin)
        {
            continue;
        }
        if (mode == 1 && unit.kind_class == 7)
        {
            continue;
        }
        if ((mode == 0 || mode == 3) && unit.same_side)
        {
            continue;
        }
        if (unit.combat_value <= minimum_value)
        {
            continue;
        }
        int d = original_distance(origin, unit.position);
        if (!best || d < distance)
        {
            best = &unit;
            distance = d;
            if (d == 1)
            {
                break;
            }
        }
    }
    return best;
}

auto maximum_route_depth(const OrderedRoutes& routes) -> int
{
    int maximum = 0;
    for (size_t i = 1; i < routes.order.size(); ++i)
    {
        int index = routes.order[i], depth = 0;
        if (!routes.records.at(index).route.marker)
        {
            continue;
        }
        while (index > 0)
        {
            index = routes.records.at(index).parent;
            ++depth;
        }
        maximum = std::max(maximum, depth);
    }
    return maximum;
}

auto target_threat(int distance, int target12c, int target130, int config_e8, int config_ee, int config_ef, int config_e9, int range, int reach, bool special) -> bool
{
    if (!config_e9)
    {
        report_error(make_error("Original AI movement divisor"));
        return false;
    }
    int budget = std::max(0, target12c + target130 - config_e8);
    int steps = std::max(0, (budget - (special ? config_ef : config_ee)) / config_e9) + (special ? 5 : range);
    return distance - steps - reach - 2 <= 0;
}

auto urgent_40CDA1(int mode, int kind, int hp, int maximum_hp, int mp, int maximum_mp, bool concealed, unsigned difficulty, int side_da, unsigned army_flags) -> bool
{
    if (maximum_hp <= 0)
    {
        return false;
    }
    if (hp * 100 / maximum_hp >= 40 && concealed)
    {
        if ((difficulty <= 2 && side_da == 1) || !(army_flags & 9)) { return true; }
    }
    int threshold;
    if (kind == 6 || kind == 7 || kind == 19 || kind == 20)
    {
        if (hp > 0 && maximum_hp > 0 && hp * 100 / maximum_hp < 10)
        {
            return false;
        }
        threshold = mode == 1 ? 70 : 90;
    }
    else if (kind == 4 || kind == 23)
    {
        if (mp > 0 && maximum_mp > 0 && mp * 100 / maximum_mp < 10)
        {
            return false;
        }
        threshold = mode == 1 ? 70 : 80;
    }
    else if (kind == 3)
    {
        threshold = mode == 1 ? 60 : 70;
    }
    else
    {
        threshold = mode == 1 ? 45 : 50;
    }
    return hp * 100 / maximum_hp >= threshold;
}

auto squared_distance(Position a, Position b) -> int
{
    int x = subtract(a.x, b.x), y = subtract(a.y, b.y);
    return std::bit_cast<int32_t>(uint32_t(multiply(x, x)) + uint32_t(multiply(y, y)));
}

auto fallback_40B0BA(unsigned status34, int mode, int health_percent) -> Fallback
{
    if (!(status34 & 0x0E))
    {
        return Fallback::default_40AED5;    // 40BCDF -> 40AED5
    }
    if (status34 & 1)
    {
        return Fallback::finish;
    }
    if (mode == 3)
    {
        return Fallback::default_40AED5;
    }
    if (mode == 1)
    {
        if (health_percent > 85)
        {
            return Fallback::approach_urgent;
        }
        if (health_percent > 75)
        {
            return Fallback::approach;
        }
        if (health_percent > 50)
        {
            return Fallback::ordinary_40B054;    // 40B054 -> 47769A
        }
    }
    else
    {
        if (health_percent > 95)
        {
            return Fallback::approach_urgent;
        }
        if (health_percent > 85)
        {
            return Fallback::approach;
        }
        if (health_percent > 60)
        {
            return Fallback::ordinary_40B054;    // 40B054 -> 47769A
        }
    }
    return Fallback::retreat;
}

auto retreat_40CFE3(int mode, int kind, int16_t hp, int16_t max_hp, int value, std::array<int, 4> components) -> bool
{
    if (max_hp <= 0)
    {
        return true;
    }
    int percentage = multiply(hp, 100) / max_hp;
    if (!value)
    {
        return false;
    }
    int weighted = multiply(components[0], 40);
    weighted = std::bit_cast<int32_t>(uint32_t(weighted) + uint32_t(multiply(components[1], mode == 1 ? 30 : 40)));
    int componentWeight;
    if (kind == 3) { componentWeight = 25; }
    else if (mode == 1) { componentWeight = 15; }
    else
    {
        componentWeight = 20;
    }
    weighted = std::bit_cast<int32_t>(uint32_t(weighted) + uint32_t(multiply(components[2], componentWeight)));
    weighted = std::bit_cast<int32_t>(uint32_t(weighted) + uint32_t(multiply(components[3], 25)));
    if (weighted == INT32_MIN && value == -1)
    {
        report_error(make_error("Original CFE3 division overflow"));
        return false;
    }
    return percentage <= weighted / value;
}

auto dispatch_40AED5(unsigned status34, unsigned status38, int runtime_kind8, int difficulty, int global4F7CD0, int side_da) -> Default40AED5
{
    bool restricted = (difficulty == 1 && global4F7CD0 == 0 && side_da == 1) || (status38 & 2) != 0;
    if ((status34 & 1) || (restricted && (status34 & 0x0E)))
    {
        return Default40AED5::finish;
    }
    if ((runtime_kind8 == 6 || runtime_kind8 == 7) && !(status38 & 2))
    {
        return Default40AED5::special_40D351;
    }
    bool goal = (status38 & 2) != 0 || (difficulty == 1 && global4F7CD0 == 0 && side_da == 1);
    return goal ? Default40AED5::goal_43939D : Default40AED5::controller_40D171;
}
}    //namespace s2ne
