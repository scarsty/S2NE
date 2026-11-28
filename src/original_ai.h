#pragma once
#include "runtime_error.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace s2ne
{
// Values are outputs of the original preview object, not rolled strike damage.
struct MeleePreview
{
    int attack_value{};                // sub_4086DF (includes sub_40883D bonus)
    int attack_chance{};               // sub_408742
    int attack_damage{};               // sub_408768
    int counter_value{};               // sub_40878E (includes sub_40883D bonus)
    int counter_chance{};              // sub_4087F1
    int counter_damage{};              // sub_408817
    int target_hp{}, attacker_hp{};    // signed runtime words +124h
    int target_kind{};                 // signed runtime word +2
    unsigned target_flags2f{};
    bool attacker_status38_02{};
};
struct Weights
{
    uint8_t priority{};    // config[100h + signed difficulty]
    uint8_t kill{};        // config[105h + signed difficulty]
};
struct Score
{
    int rank{}, value{}, kills{};
};
int multiply(int a, int b);
int subtract(int a, int b);
// 4086DF multiplies preview chance (+18) by damage (+1C); no /100.
int physical_preview_value(int chance, int damage, bool status_bonus = false);
// sub_40BCF3, non-area branch. A strict > test is intentional at both boundaries.
std::optional<Score> score_melee(const MeleePreview& p, Weights w);
// 原40814F物攻候选比较，允许已有rank3恢复候选；其它技能rank仍由各族比较器处理。
bool improves_melee(Score candidate, Score current);
bool improves_skill(Score candidate, Score current, int threshold);
struct Candidate
{
    std::size_t target{};        // opaque caller token; no assumption about unit ID/index
    int destination_record{};    // zero=current position, otherwise original record index+1
    MeleePreview preview{};
    int action_index{ -1 };    // Original action-definition token retained by executable controller.
};
struct Selection
{
    Score score{};
    std::optional<Candidate> candidate;
};
// Call in original action-definition order, current position then ordered routes.
// Each call supplies ALL targets at one position in original enumeration order.
// true means stop the outer position/action scan (40B6B2 / 40B78F).
bool scan_melee_position(std::span<const Candidate> candidates, Weights weights, Selection& selected, int skill_threshold = 0);
// sub_40B6D2: +7 is signed and only tested for zero; +12h is unsigned.
bool melee_route_candidate(int signed_record7, unsigned category12, bool movement_used);
// 原40B538在全部技能扫描结束后才过滤，不在各候选阶段提前丢弃。
bool skill_route_allowed(const Score& score, int movement_mode);
// sub_40C906 dispatch only; inputs MUST be results of its two original helpers.
int movement_mode(bool helper40CAFE, bool helper40C956);

struct Position
{
    int x{}, y{};
    bool operator==(const Position&) const = default;
};
// Literal bounded ring walk of sub_478AF3, including same-cell result 80.
int original_distance(Position from, Position to);
struct RouteRecord
{
    int index{};    // actual zero-based record index, NOT ordered-table position
    Position position{};
    int remaining{};        // signed word +4
    int marker{};           // signed byte +7, only zero is rejected
    unsigned category{};    // byte +12h; not filtered by fallback selectors
};
struct RouteNode
{
    RouteRecord route{};
    int parent{};
    std::array<int, 4> children{ -1, -1, -1, -1 };    // record +Ah/+Ch/+Eh/+10h
    bool live = true;
};
template <class Scan>
bool scan_action_positions(std::span<const RouteRecord> routes, bool moved, Scan scan, Position origin)
{
    if (scan(origin, 0)) { return true; }
    for (const auto& record : routes)
    {
        if (melee_route_candidate(record.marker, record.category, moved) && scan(record.position, record.index + 1)) { return true; }
    }
    return false;
}
struct OrderedRoutes
{
    std::vector<RouteNode> records;
    std::vector<int> order;    // includes original slot zero
    std::vector<RouteRecord> candidates() const;
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
            result.records.push_back({});
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
    std::size_t token{};
    Position position{};
    bool runtime_ad_nonzero{};
    unsigned status33{};
    int kind_class{};      // signed kind-table byte +11h
    int combat_value{};    // sub_45CFBB with both selector arguments zero
    bool same_side{};      // Legacy field name: sub_441020 == 1 means HIDDEN, not same allegiance.
};
// sub_4394ED / 43948D. Caller passes list order, not sorted targets.
const MovementTarget* original_target(std::span<const MovementTarget> units, Position origin, int mode, int minimum_value = 0);
// 4775F5 returns maximum parent-chain length, NOT AP or geometric radius.
int maximum_route_depth(const OrderedRoutes& routes);
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
bool target_threat(int distance, int target12c, int target130, int config_e8, int config_ee, int config_ef, int config_e9, int range, int reach, bool special);
// 40CDA1 checks concealment before class thresholds; +122 SAR16 reads WORD +124.
bool urgent_40CDA1(int mode, int kind, int hp, int maximum_hp, int mp, int maximum_mp, bool concealed = false, unsigned difficulty = 3, int side_da = 0, unsigned army_flags = 0);
struct MoveSelection
{
    int destination_record{};
    Position destination{};
    int distance{};
};
int squared_distance(Position a, Position b);
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
Fallback fallback_40B0BA(unsigned status34, int mode, int health_percent);
// 40CFE3: component values are45CFBB modes5..8, with native dword wrap.
bool retreat_40CFE3(int mode, int kind, int16_t hp, int16_t max_hp, int value, std::array<int, 4> components);
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
Default40AED5 dispatch_40AED5(unsigned status34, unsigned status38, int runtime_kind8, int difficulty, int global4F7CD0, int side_da);
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
}    //namespace s2ne
