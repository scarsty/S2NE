#pragma once
#include "runtime_error.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace s2ne::original_ai
{
// Values are outputs of the original preview object, not rolled strike damage.
struct MeleePreview
{
    int attack_value{ };                 // sub_4086DF (includes sub_40883D bonus)
    int attack_chance{ };                // sub_408742
    int attack_damage{ };                // sub_408768
    int counter_value{ };                // sub_40878E (includes sub_40883D bonus)
    int counter_chance{ };               // sub_4087F1
    int counter_damage{ };               // sub_408817
    int target_hp{ }, attacker_hp{ };    // signed runtime words +124h
    int target_kind{ };                  // signed runtime word +2
    unsigned target_flags2f{ };
    bool attacker_status38_02{ };
};
struct Weights
{
    uint8_t priority{ };    // config[100h + signed difficulty]
    uint8_t kill{ };        // config[105h + signed difficulty]
};
struct Score
{
    int rank{ }, value{ }, kills{ };
};
inline int multiply(int a, int b) { return std::bit_cast<int32_t>(uint32_t(a) * uint32_t(b)); }
inline int subtract(int a, int b) { return std::bit_cast<int32_t>(uint32_t(a) - uint32_t(b)); }
// 4086DF multiplies preview chance (+18) by damage (+1C); no /100.
inline int physical_preview_value(int chance, int damage, bool status_bonus = false) { return multiply(chance + (status_bonus ? 10 : 0), damage); }
// sub_40BCF3, non-area branch. A strict > test is intentional at both boundaries.
inline std::optional<Score> score_melee(const MeleePreview& p, Weights w)
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
// Exact sub_40814F subset for an initially empty, ordinary-melee-only scan.
// No abilities/area candidates may be mixed into this state.
inline bool improves_melee(Score candidate, Score current)
{
    if (candidate.rank != 1 && candidate.rank != 2)
    {
        return false;
    }
    if (current.rank == 0)
    {
        return true;
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
struct Candidate
{
    std::size_t target{ };        // opaque caller token; no assumption about unit ID/index
    int destination_record{ };    // zero=current position, otherwise original record index+1
    MeleePreview preview{ };
    int action_index{ -1 };    // Original action-definition token retained by executable controller.
};
struct Selection
{
    Score score{ };
    std::optional<Candidate> candidate;
};
// Call in original action-definition order, current position then ordered routes.
// Each call supplies ALL targets at one position in original enumeration order.
// true means stop the outer position/action scan (40B6B2 / 40B78F).
inline bool scan_melee_position(std::span<const Candidate> candidates, Weights weights, Selection& selected)
{
    for (const auto& candidate : candidates)
    {
        auto score = score_melee(candidate.preview, weights);
        if (score && improves_melee(*score, selected.score))
        {
            selected.score = *score;
            selected.candidate = candidate;
        }
    }
    return selected.score.rank == 1;
}
// sub_40B6D2: +7 is signed and only tested for zero; +12h is unsigned.
inline bool melee_route_candidate(int signed_record7, unsigned category12, bool movement_used)
{
    return !movement_used && signed_record7 != 0 && category12 >= 1;
}
// sub_40C906 dispatch only; inputs MUST be results of its two original helpers.
inline int movement_mode(bool helper40CAFE, bool helper40C956)
{
    if (helper40CAFE) { return 0; }
    else if (helper40C956) { return 1; }
    else
    {
        return 3;
    }
}

struct Position
{
    int x{ }, y{ };
    bool operator==(const Position&) const = default;
};
// Literal bounded ring walk of sub_478AF3, including same-cell result 80.
inline int original_distance(Position from, Position to)
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
struct RouteRecord
{
    int index{ };    // actual zero-based record index, NOT ordered-table position
    Position position{ };
    int remaining{ };        // signed word +4
    int marker{ };           // signed byte +7, only zero is rejected
    unsigned category{ };    // byte +12h; not filtered by fallback selectors
};
struct RouteNode
{
    RouteRecord route{ };
    int parent{ };
    std::array<int, 4> children{ -1, -1, -1, -1 };    // record +Ah/+Ch/+Eh/+10h
    bool live = true;
};
struct OrderedRoutes
{
    std::vector<RouteNode> records;
    std::vector<int> order;    // includes original slot zero
    std::vector<RouteRecord> candidates() const
    {
        std::vector<RouteRecord> result;
        for (std::size_t i = 1; i < order.size(); ++i)
        {
            result.push_back(records.at(order[i]).route);
        }
        return result;
    }
};
// Pure graph part of 476707/47797C/477BEB/478191. step(from,to) supplies
// the original terrain/side gates and NONNEGATIVE spent cost, or nullopt.
// marker(to) supplies the original landing marker (0 may still expand).
// budget() supplies already signed-word-normalized AP minus config+E8.
// classify(remaining) supplies original +12h classification.
// Dword load at +8 followed by SAR16 reads the first child at +Ah.
template <class Step, class Marker, class Budget, class Classify>
OrderedRoutes build_ordered_routes(Position origin, Step step, Marker marker, Budget budget, Classify classify)
{
    OrderedRoutes result;
    result.records.reserve(2522);
    result.order.reserve(2522);
    std::vector<int> recycled;
    int initial = std::min(110, int(budget()));
    result.records.push_back({ { 0, origin, initial, 1, 0 }, 0 });
    result.order.push_back(0);
    auto invalidate = [&](auto&& self, int index) -> void
    {
        if (index < 0 || !result.records.at(index).live)
        {
            return;
        }
        auto children = result.records[index].children;
        for (int child : children)
        {
            if (child >= 0)
            {
                self(self, child);
            }
        }
        result.records[index].live = false;
    };
    auto compact = [&]()
    {
        // 4782FA: scan from slot1; pop dead tail, then fill dead slot with
        // live tail. Slots skipped after swap are intentional.
        for (std::size_t i = 1; i < result.order.size(); ++i)
        {
            if (result.records[result.order[i]].live)
            {
                continue;
            }
            do
            {
                int tail = result.order.back();
                result.order.pop_back();
                if (!result.records[tail].live) { recycled.push_back(tail); }
                else
                {
                    recycled.push_back(result.order[i]);
                    result.order[i] = tail;
                    break;
                }
            } while (i < result.order.size());
        }
    };
    auto insert = [&](int parent, Position to) -> int
    {
        auto source = result.records.at(parent).route;
        if (source.remaining <= 0 || to == origin)
        {
            return -1;
        }
        auto cost = step(source.position, to);
        if (!cost)
        {
            return -1;
        }
        if (*cost < 0)
        {
            return -1;
        }
        int remaining = source.remaining - *cost;
        if (remaining < 0)
        {
            return -1;
        }
        // 477BEB computes landing marker before 478191 duplicate handling.
        int landing = marker(to);
        for (std::size_t i = 1; i < result.order.size(); ++i)
        {
            int old = result.order[i];
            if (result.records[old].route.position != to)
            {
                continue;
            }
            if (result.records[old].route.remaining >= remaining)
            {
                return -1;
            }
            int previous = result.records[old].parent;
            auto& siblings = result.records.at(previous).children;
            auto child = std::find(siblings.begin(), siblings.end(), old);
            if (child == siblings.end())
            {
                return -1;
            }
            *child = -1;
            invalidate(invalidate, old);
            compact();
            break;
        }
        int index;
        if (recycled.empty())
        {
            index = int(result.records.size());
            if (index >= 2522)
            {
                return -1;
            }
            result.records.push_back({ });
        }
        else
        {
            index = recycled.back();
            recycled.pop_back();
        }
        result.records[index] = { { index, to, remaining, landing, unsigned(classify(remaining)) }, parent, { -1, -1, -1, -1 }, true };
        result.order.push_back(index);
        return index;
    };
    auto expand = [&](auto&& self, int index) -> void
    {
        Position p = result.records.at(index).route.position;
        Position previous = result.records.at(result.records[index].parent).route.position;
        int left = p.x - ((p.y & 1) ? 0 : 1), right = left + 1;
        std::array<Position, 4> next{ { { right, p.y - 1 }, { left, p.y + 1 }, { left, p.y - 1 }, { right, p.y + 1 } } };
        for (int direction = 0; direction < 4; ++direction)
        {
            if (next[direction] == previous)
            {
                result.records[index].children[direction] = -1;
                continue;
            }
            int child = insert(index, next[direction]);
            result.records[index].children[direction] = child;
            if (child >= 0)
            {
                self(self, child);
            }
        }
    };
    expand(expand, 0);
    return result;
}
struct MovementTarget
{
    std::size_t token{ };
    Position position{ };
    bool runtime_ad_nonzero{ };
    unsigned status33{ };
    int kind_class{ };      // signed kind-table byte +11h
    int combat_value{ };    // sub_45CFBB with both selector arguments zero
    bool same_side{ };      // Legacy field name: sub_441020 == 1 means HIDDEN, not same allegiance.
};
// sub_4394ED / 43948D. Caller passes list order, not sorted targets.
inline const MovementTarget* original_target(std::span<const MovementTarget> units, Position origin, int mode, int minimum_value = 0)
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
// 4775F5 returns maximum parent-chain length, NOT AP or geometric radius.
inline int maximum_route_depth(const OrderedRoutes& routes)
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
// 40CAFE visits rings 1..reach+1, with a strict combat-value comparison.
// Lookup is 446E53(mode 1), so the adapter must apply visibility first.
template <class Lookup>
bool nearby_threat(Position origin, int reach, int own_value, Lookup lookup)
{
    if (reach <= 0)
    {
        return false;
    }
    Position top = origin;
    for (int radius = 1; radius <= reach + 1; ++radius)
    {
        top.x -= (top.y & 1) ? 0 : 1;
        --top.y;
        auto threat = [&](Position p)
        {
            auto value = lookup(p);
            return value && *value > own_value / 3;
        };
        for (int x = top.x; x <= top.x + radius; ++x)
        {
            if (threat({ x, top.y }) || threat({ x, top.y + 2 * radius }))
            {
                return true;
            }
        }
        for (int y = top.y + 2; y <= top.y + 2 * radius - 2; y += 2)
        {
            if (threat({ top.x, y }) || threat({ top.x + radius, y }))
            {
                return true;
            }
        }
    }
    return false;
}
// 40C956: +12C/+130 signed words, E8/EE/EF/E9 signed config bytes;
// special is actor kind6/7 AND target's 44314E result exactly2.
inline bool target_threat(int distance, int target12c, int target130, int config_e8, int config_ee, int config_ef, int config_e9, int range, int reach, bool special)
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
// 40CDA1 ordinary non-hidden branch. +122 SAR16 reads WORD +124.
inline bool urgent_40CDA1(int mode, int kind, int hp, int maximum_hp, int mp, int maximum_mp)
{
    if (maximum_hp <= 0)
    {
        return false;
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
struct MoveSelection
{
    int destination_record{ };
    Position destination{ };
    int distance{ };
};
inline int squared_distance(Position a, Position b)
{
    int x = subtract(a.x, b.x), y = subtract(a.y, b.y);
    return std::bit_cast<int32_t>(uint32_t(multiply(x, x)) + uint32_t(multiply(y, y)));
}
// sub_4773A3 / 47736A. routes excludes ordered-table slot zero.
// occupied implements 42CCFD(map,x,y,actor)==1; adjacent implements 4421BD.
// Distance is measured FROM target, preserving the original call direction.
template <class Occupied, class Adjacent>
std::optional<MoveSelection> select_toward(std::span<const RouteRecord> routes, Position target, int reserve, Occupied occupied, Adjacent adjacent)
{
    std::optional<MoveSelection> best;
    for (const auto& r : routes)
    {
        if (!r.marker || occupied(r.position))
        {
            continue;
        }
        if (reserve > 0 && (r.remaining < reserve || adjacent(r.position)))
        {
            continue;
        }
        int distance = original_distance(target, r.position);
        if (!best || distance < best->distance)
        {
            best = MoveSelection{ r.index + 1, r.position, distance };
            if (distance == 1)
            {
                break;
            }
        }
        else if (distance == best->distance && squared_distance(target, r.position) < squared_distance(target, best->destination))
        {
            best = MoveSelection{ r.index + 1, r.position, distance };
        }
    }
    return best;
}
// 40D25A chooses mode-0 target and uses signed config +ED/+F0 bytes.
template <class Occupied, class Adjacent>
std::optional<MoveSelection> select_40D25A(std::span<const RouteRecord> routes, std::span<const MovementTarget> targets, Position origin, bool urgent, int config_ed, int config_f0, Occupied occupied, Adjacent adjacent)
{
    auto target = original_target(targets, origin, 0);
    if (!target)
    {
        return std::nullopt;
    }
    return select_toward(routes, target->position, config_ed + (urgent ? 0 : config_f0), occupied, adjacent);
}
// 4421BD tests four diagonal neighbors in this order, via side+282h's
// 446E53(...,0)>0. This is NOT a generic occupied/visible-cell predicate.
template <class SideLookup>
bool original_adjacent(Position p, SideLookup side_lookup)
{
    int left = p.x - ((p.y & 1) ? 0 : 1), right = left + 1;
    return side_lookup(Position{ right, p.y - 1 }) > 0 || side_lookup(Position{ left, p.y + 1 }) > 0 || side_lookup(Position{ left, p.y - 1 }) > 0 || side_lookup(Position{ right, p.y + 1 }) > 0;
}
// sub_47769A, used by 40B054. The secondary list uses mode 1. The
// secondary-distance local intentionally persists when its lookup is empty.
template <class Adjacent>
std::optional<MoveSelection> select_47769A(std::span<const RouteRecord> routes, std::span<const MovementTarget> primary, std::span<const MovementTarget> secondary, Position origin, Adjacent adjacent, const MovementTarget* explicit_target = nullptr)
{
    auto target = explicit_target ? explicit_target : original_target(primary, origin, 2);
    if (!target)
    {
        return std::nullopt;
    }
    int initial = original_distance(target->position, origin), best_secondary = 0, secondary_distance = 0;
    std::optional<MoveSelection> best;
    for (const auto& r : routes)
    {
        if (!r.marker || adjacent(r.position))
        {
            continue;
        }
        int d = original_distance(target->position, r.position);
        if (auto nearest_target = original_target(secondary, r.position, 1))
        {
            secondary_distance = original_distance(nearest_target->position, r.position);
        }
        bool replace = !best;
        if (best && d < best->distance)
        {
            replace = secondary_distance > best_secondary || initial > best->distance || secondary_distance > 10;
        }
        if (replace)
        {
            best = MoveSelection{ r.index + 1, r.position, d };
            best_secondary = secondary_distance;
        }
    }
    return best;
}
// sub_40B0BA's final ordinary fallback dispatch after its earlier special
// exits. The caller computes 40C906 and the signed health percentage.
enum class Fallback
{
    finish,
    default_40AED5,
    ordinary_40B054,
    approach,
    retreat,
    approach_urgent
};
inline Fallback fallback_40B0BA(unsigned status34, int mode, int health_percent)
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
// 40CFE3: component values are45CFBB modes5..8, with native dword wrap.
inline bool retreat_40CFE3(int mode, int kind, int16_t hp, int16_t max_hp, int value, std::array<int, 4> components)
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
// 40D171 behavior codes are the original local values, not action IDs.
enum class Behavior40D171
{
    retreat = 1,
    approach = 2,
    approach_urgent = 3
};
// Callbacks preserve original short-circuit order and must return the raw
// helper result: only exactly 1 counts as true. Mode 3 calls neither helper.
template <class CDA1, class CFE3>
Behavior40D171 dispatch_40D171(int mode, int16_t runtime124, int16_t runtimeB7, CDA1 cda1, CFE3 cfe3)
{
    if (mode == 3)
    {
        return runtime124 < runtimeB7 ? Behavior40D171::approach : Behavior40D171::approach_urgent;
    }
    if (cda1(mode) == 1)
    {
        return Behavior40D171::approach_urgent;
    }
    if (cfe3(mode) == 1)
    {
        return Behavior40D171::retreat;
    }
    return Behavior40D171::approach;
}
// 40AED5 entry, including its non-generic special-goal and kind branches.
// 4288A6 ->44367C initializes allied army +DA=0, hostile army +DA=1.
// Do not derive this from the active round/banner or a target's allegiance.
enum class Default40AED5
{
    finish,
    special_40D351,
    goal_43939D,
    controller_40D171
};
inline Default40AED5 dispatch_40AED5(unsigned status34, unsigned status38, int runtime_kind8, int difficulty, int global4F7CD0, int side_da)
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
// sub_40C659, used by retreat wrapper 40C425. nearest_at must perform
// 43948D(mode 0) at the candidate position. An empty lookup aborts and
// clears the result, exactly like 40C7E9's predecessor.
template <class Occupied, class NearestAt>
std::optional<MoveSelection> select_retreat(std::span<const RouteRecord> routes, bool prefer_reserve, int reserve, Occupied occupied, NearestAt nearest_at)
{
    std::optional<MoveSelection> best, reserved;
    for (const auto& r : routes)
    {
        if (!r.marker || occupied(r.position))
        {
            continue;
        }
        auto target = nearest_at(r.position);
        if (!target)
        {
            return std::nullopt;
        }
        int d = original_distance(r.position, target->position);
        if (prefer_reserve && r.remaining >= reserve && (!reserved || d > reserved->distance))
        {
            reserved = MoveSelection{ r.index + 1, r.position, d };
        }
        if (!best || d > best->distance)
        {
            best = MoveSelection{ r.index + 1, r.position, d };
        }
    }
    return prefer_reserve && reserved ? reserved : best;
}
}    //namespace s2ne::original_ai
