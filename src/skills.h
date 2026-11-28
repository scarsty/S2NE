#pragma once
#include "ability_buffs.h"
#include "combat.h"
#include "damage_records.h"
#include "drain.h"
#include "effect55.h"
#include "events.h"
#include "hostile_status.h"
#include "menu.h"
#include "palette.h"
#include "promotion_transaction.h"
#include "skill94_formula.h"
#include "sleep2.h"
#include "special_status.h"
#include "sprite.h"
#include "timed_buffs.h"
#include <functional>
#include <optional>

namespace s2ne
{
// 464C0C/464C83 -> 469F04. Selection requests are not execution results.
class Skills
{
public:
    enum class Page
    {
        list,
        requested,
        closed
    };
    enum class Failure
    {
        none,
        mp,
        st,
        action_points,
        hp,
        partner_unresolved,
        world_unresolved,
        unsupported_execution,
        stale_unit
    };
    struct Record
    {
        unsigned id{}, group{}, bit{};
        std::array<uint8_t, 37> raw{};
        int word(size_t offset) const;
        std::vector<uint8_t> name() const;
        unsigned effect() const;
    };
    struct Group
    {
        unsigned index{};
        std::vector<uint8_t> name;
        std::vector<Record> records;
    };
    struct Context
    {
        bool second{}, suppress_first{}, suppress_second{}, army_da_nonzero{};
        bool status3d_40{};    // Original full runtime status unavailable in Unit.status3c byte.
        bool debug{}, computer{}, item_cast{};
    };
    struct Request
    {
        unsigned unit_id{}, appearance{}, skill_id{}, group{}, bit{};
        bool second{};
        unsigned partner_id{}, partner_appearance{};
    };
    struct Result
    {
        bool applied{};
        Failure failure{ Failure::unsupported_execution };
        int healing{}, experience_award{}, proficiency_award{};
    };
    struct ExecutionContext
    {
        unsigned difficulty{ 3 };
        uint32_t seconds{};
        uint32_t status3c{};               // Full runtime +3C DWORD, not Unit.status3c's low byte.
        unsigned skill_payment_state{};    // Live 505C1C[skill-1], initially BIN10 table2.
        bool friendly_army{ true }, conditions_mode{};
    };
    // Apply proficiency/learning (group), then 458B03/459309 XP/level-up to
    // the staged unit/RNG only. Return false when the native bridge is absent.
    using AwardAdapter = std::function<bool(Events::Unit&, s2ne::Random&, int, int, unsigned)>;
    using SummonAdapter = std::function<bool(const Events::Unit&, unsigned, s2ne::Random&)>;
    static int summon_level(const Events::Unit& caster);

private:
    Menu font_;
    SharedBytes bin_, pal_, cel_;
    bool inspection_help_{};
    std::vector<uint8_t> inspection_description_;
    std::vector<Group> groups_;
    size_t header_{};
    int ap_base_{}, ap_repeat_{};
    std::array<int, 256> config_{};
    unsigned ai_status_weight_{};
    unsigned owner_id_{}, owner_appearance_{};
    Context context_;
    bool item_source_{};
    std::pair<int, int> area_camera_{};
    Page page_{ Page::closed };
    size_t group_{}, skill_{};
    std::optional<Request> request_;
    size_t entry(unsigned id) const;
    std::vector<uint8_t> string(size_t p) const;
    s2ne::Result<bool> try_sprite(std::vector<uint8_t>& image, unsigned index, int left, int top) const;

public:
    Skills() = default;
    static s2ne::Result<Skills> create(const std::filesystem::path& root, bool small_font = false);
    explicit Skills(const std::filesystem::path& root, bool small_font = false);
    int action_cost(const Events::Unit& unit) const;
    bool entry_available(const Events::Unit& unit, Context context) const;
    void open(const Events::Unit& unit, Context context, bool inspection = false);
    const std::vector<Group>& groups() const;
    bool inspection_available(const Events::Unit& unit, bool second) const;
    void open_inspection(const Events::Unit& unit, bool second);
    s2ne::Result<bool> try_draw_status_effects(std::vector<uint8_t>& image, const Events::Unit& unit) const;
    s2ne::Result<bool> try_draw_inspection(std::vector<uint8_t>& image, const Events::Unit& unit, FullWidthCanvas* canvas = nullptr) const;
    bool inspection_help() const;
    void close_inspection_help();
    s2ne::Result<bool> try_open_inspection_help();
    Page page() const;
    void navigate_inspection(unsigned scan, bool change_group);
    bool mouse_inspection(int x, int y, bool click);
    size_t selected_group() const;
    size_t selected_skill() const;
    const Record& selected() const;
    bool select_record(unsigned id);
    // 原401146先自身，再逐圈上下交替、左右交替；空格也必须参与面积中心评分。
    template <class Valid>
    std::vector<std::pair<int, int>> ai_centers(const Events::Unit& actor, Valid valid) const
    {
        std::vector<std::pair<int, int>> cells{ { actor.x, actor.y } };
        int radius = selected().raw[17];
        if (selected().id == 88 || selected().raw[18] == 5 || selected().raw[18] == 8) { radius = 0; }
        int x = actor.x, y = actor.y;
        for (int r = 1; r <= radius; ++r)
        {
            if (!(y & 1)) { --x; }
            --y;
            for (int col = x; col <= x + r; ++col)
            {
                if (valid(col, y)) { cells.emplace_back(col, y); }
                if (valid(col, y + 2 * r)) { cells.emplace_back(col, y + 2 * r); }
            }
            for (int row = y + 2; row <= y + 2 * r - 2; row += 2)
            {
                if (valid(x, row)) { cells.emplace_back(x, row); }
                if (valid(x + r, row)) { cells.emplace_back(x + r, row); }
            }
        }
        return cells;
    }
    bool directional_cell_in_range(const Events::Unit& actor, int x, int y) const;
    // 原443235只查学习组；470EF6才检查AP、禁用与3D40，不能借玩家菜单可用性代替。
    bool ai_group_available(const Events::Unit& unit, Context context) const;
    s2ne::Result<int> ai_healing_value(const Events::Unit& actor, const Events::Unit& target,
        const Events& events, unsigned priority, const Events::Unit* partner = nullptr) const;
    bool ai_summon_available(const Events::Unit& actor, const Events& events) const;
    bool ai_status_supported() const;
    bool ai_original_unselected() const;
    // 原471585免疫／已有状态门控，4714E4使用WORD35而不是效果威力。
    std::pair<int, int> ai_status_value(const Events::Unit& actor, const Events::Unit& target,
        const Events& events) const;
    Failure resource_failure(const Events::Unit& unit, const Record& record) const;
    template <class Elevation>
    std::optional<size_t> cooperative_partner(const Events::Unit& actor, const std::vector<Events::Unit>& units, unsigned turn, Elevation elevation) const
    {
        const auto& record = selected();
        if (!(record.raw[32] & 7)) { return std::nullopt; }
        // 原+38h是4651EA道具施法模式，不是角色／地图字段；普通4650B4置0。
        if (context_.item_cast) { return std::nullopt; }
        const int right = actor.x + int(actor.y & 1), left = right - 1;
        const std::array<std::pair<int, int>, 4> positions{ { { right, actor.y - 1 }, { left, actor.y + 1 }, { left, actor.y - 1 }, { right, actor.y + 1 } } };
        for (auto [x, y] : positions)
        {
            for (size_t i = 0; i < units.size(); ++i)
            {
                const auto& partner = units[i];
                if (partner.first_turn > int(turn) || (partner.last_turn != 0 && partner.last_turn < int(turn)) || !(partner.status_flags & 0x80) || (partner.status_flags & 0x20) || partner.x != x || partner.y != y || partner.hostile() != actor.hostile()) { continue; }
                // 45CC0C returns the first occupied slot; 46AAEB rejects hidden +2D.
                if (partner.action_state != 0 || !(partner.learned[record.group] & (1u << record.bit)) || std::abs(elevation(actor.x, actor.y) - elevation(x, y)) > 3) { break; }
                if (record.raw[32] & 6)
                {
                    if (actor.id < 1 || actor.id > 40 || partner.id < 1 || partner.id > 41) { break; }
                    if (partner.id > 40 || actor.relationships[partner.id - 1] < 70 || partner.relationships[actor.id - 1] < 70) { break; }
                }
                return i;
            }
        }
        return std::nullopt;
    }
    bool cooperative_pair(const Events::Unit& actor, const Events::Unit& partner) const;
    Failure select(const Events::Unit& unit, const Events::Unit* partner = nullptr, bool summon_world_ready = false);
    const std::optional<Request>& request() const;
    bool item_source() const;
    std::vector<Record> newly_learned_records(const Events::Unit& before, const Events::Unit& after) const;
    bool source_authorized(const Events::Unit& unit, unsigned group, unsigned bit) const;
    bool open_item(const Events::Unit& unit, unsigned id);
    s2ne::Result<unsigned> try_initial_payment_state(unsigned id) const;
    unsigned initial_payment_state(unsigned id) const;
    bool effect_supported(const Record& record) const;
    bool executable_available(const Events::Unit& unit, Context context, bool award_bridge_ready) const;
    Result execute(Events::Unit&) const;
    Result execute(Events::Unit& unit, const Events::Unit& target, const Events& events,
        s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    bool healing1_supported() const;
    bool healing1_in_range(const Events::Unit& actor, const Events::Unit& target) const;
    bool selection_cell_in_range(const Events::Unit& actor, int x, int y) const;
    Result execute_healing1(Events::Unit& actor, Events::Unit& target, const Events& events,
        s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    struct HostileResult
    {
        bool applied{}, hit{}, immune{};
        unsigned removed{}, slot{};
        int chance{}, roll{};
        Failure failure{ Failure::none };
        std::vector<bool> target_hits;
        std::vector<int> target_amounts;
        int raw_xp{}, proficiency{};
        bool reveal_army{};
        std::vector<Drain::Packet> drain_targets;
    };
    bool sword94_supported() const;
    bool damage_supported() const;
    bool drain_supported() const;
    bool fissure29_supported() const;
    bool transfer51_supported() const;
    bool summon_supported() const;
    bool curse33_supported() const;
    bool sleep2_supported() const;
    bool hostile_stat_supported() const;
    bool special_status_supported() const;
    bool hostile_status_supported() const;
    bool blessing3_supported() const;
    bool strengthening4_supported() const;
    bool protection5_supported() const;
    bool restoration_supported() const;
    bool area_healing_supported() const;
    bool timed_buff_supported() const;
    bool cooperative_supported() const;
    bool ability_buff_supported() const;
    bool invisibility47_supported() const;
    bool reveal50_supported() const;
    bool spirit45_supported() const;
    bool friendly_status_supported() const;
    void pay_staged(Events::Unit& actor, ExecutionContext context, uint32_t payment_abilities) const;
    bool status_area_supported() const;
    bool status_area_target(const Events::Unit& actor, const Events::Unit& target) const;
    bool sleep2_center_in_range(const Events::Unit& actor, const Events::Unit& center) const;
    bool directional_skill() const;
    void set_area_camera(std::pair<int, int> camera);
    std::function<int(int, int)> area_elevation_;
    std::function<bool(int, int)> area_valid_;
    void set_area_elevation(std::function<int(int, int)> elevation);
    void set_area_valid(std::function<bool(int, int)> valid);
    std::optional<unsigned> ai_direction_;
    void set_ai_direction(std::optional<unsigned> direction);
    std::vector<std::pair<int, int>> ai_direction_cells(const Events::Unit& center, unsigned direction) const;
    std::vector<std::pair<int, int>> radial_cells(const Events::Unit& center, bool preserve_ray_slots = false) const;
    bool viewport_area_cell(int x, int y) const;
    bool sleep2_area(const Events::Unit& center, const Events::Unit& target) const;
    s2ne::Result<HostileResult> execute_sleep2(Events::Unit& actor, std::vector<Events::Unit>& targets,
        const Events::Unit& center, s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    s2ne::Result<HostileResult> execute_blessing3(Events::Unit& actor, std::vector<Events::Unit>& targets,
        const Events::Unit& center, s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    s2ne::Result<HostileResult> execute_strengthening_status(Events::Unit& actor, std::vector<Events::Unit>& targets,
        const Events::Unit& center, const Events& events, s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    s2ne::Result<HostileResult> execute_restoration(Events::Unit& actor, std::vector<Events::Unit>& targets,
        const Events::Unit& center, const Events& events, s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    bool hostile_supported() const;
    template <class ValidCell, class Visible>
    bool selection_has_target(const Events::Unit& actor, const std::vector<Events::Unit>& units, unsigned turn, ValidCell valid, Visible visible) const
    {
        if (summon_supported() || selected().id == 88 || transfer51_supported() || spirit45_supported() || reveal50_supported()) { return true; }
        const bool area = status_area_supported();
        if (area && selected().raw[31] != 2 && selected().raw[31] != 3) { return true; }
        if (area)
        {
            const int radius = selected().raw[17];
            for (int y = actor.y - radius; y <= actor.y + radius; ++y)
            {
                for (int x = actor.x - radius; x <= actor.x + radius; ++x)
                {
                    if (!valid(x, y) || !selection_cell_in_range(actor, x, y)) { continue; }
                    auto center = actor;
                    center.x = x;
                    center.y = y;
                    for (size_t target : collect_targets(actor, center, units, turn, valid))
                    {
                        if (visible(units[target])) { return true; }
                    }
                }
            }
            return false;
        }
        return std::any_of(units.begin(), units.end(), [&](const auto& target)
            {
                return target.present(turn) && valid(target.x, target.y) && visible(target) && (healing1_in_range(actor, target) || hostile_in_range(actor, target));
            });
    }
    // 401146 builds concentric diamond rings; record17 is the radius, not AP.
    bool hostile_in_range(const Events::Unit& actor, const Events::Unit& target) const;
    HostileResult execute_hostile(Events::Unit& actor, Events::Unit& target,
        s2ne::Random& random, ExecutionContext context, const PromotionTransaction& equipment, const AwardAdapter& award);
    s2ne::Result<HostileResult> execute_curse33(Events::Unit& actor, Events::Unit& target,
        s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    enum class Handler
    {
        unavailable,
        damage466082,
        healing465C14,
        status4668F0,
        hostile467625,
        equipment467625,
        drain467625,
        fissure467625,
        transfer4665D8,
        summon465E04,
        specialStatus,
        army4666C5,
        spirit46864F
    };
    Handler handler(bool summon_ready = false) const;
    bool pipeline_supported() const;
    template <class ValidCell>
    std::vector<size_t> collect_targets(const Events::Unit& actor, const Events::Unit& center,
        const std::vector<Events::Unit>& units, unsigned turn, ValidCell valid) const
    {
        std::vector<size_t> out;
        if (selected().raw[18] == 5 || selected().raw[18] == 6 || selected().raw[18] == 8)
        {
            const bool directed = ai_direction_ && (selected().raw[18] == 5 || selected().raw[18] == 8);
            for (auto [x, y] : radial_cells(directed ? actor : center))
            {
                if (!valid(x, y)) { continue; }
                for (size_t n = 0; n < units.size(); ++n)
                {
                    const auto& u = units[n];
                    if (u.x == x && u.y == y && u.present(turn) && u.hp > 0 && (u.status_flags & 0x80) && !(u.status_flags & 0x20) && status_area_target(actor, u) && std::find(out.begin(), out.end(), n) == out.end()) { out.push_back(n); }
                }
            }
            return out;
        }
        if (selected().raw[18] == 7)
        {
            const int firstRow = area_camera_.second / 16;
            for (int y = firstRow; y < firstRow + 36; ++y)
            {
                for (int x = area_camera_.first / 64 - 1; x <= area_camera_.first / 64 + 11; ++x)
                {
                    if (!viewport_area_cell(x, y) || !valid(x, y)) { continue; }
                    for (size_t n = 0; n < units.size(); ++n)
                    {
                        const auto& u = units[n];
                        if (u.x == x && u.y == y && u.present(turn) && u.hp > 0 && (u.status_flags & 0x80) && !(u.status_flags & 0x20) && status_area_target(actor, u)) { out.push_back(n); }
                    }
                }
            }
            return out;
        }
        const int side = 2 * int(selected().raw[18]) - 1;
        int startX = center.x, startY = center.y - side + 1;
        for (int row = 0; row < side; ++row)
        {
            int x = startX, y = startY;
            for (int col = 0; col < side; ++col)
            {
                if (valid(x, y))
                {
                    for (size_t n = 0; n < units.size(); ++n)
                    {
                        const auto& u = units[n];
                        const bool friendly = selected().raw[31] != 2;
                        const bool self = selected().id != 88 || (u.id == actor.id && u.appearance == actor.appearance);
                        if (u.x == x && u.y == y && u.present(turn) && u.hp > 0 && (u.status_flags & 0x80) && !(u.status_flags & 0x20) && self && (!damage_supported() || u.id != actor.id || u.appearance != actor.appearance) && (selected().raw[31] == 0 || selected().raw[31] == 3 || ((u.hostile() == actor.hostile()) == friendly))) { out.push_back(n); }
                    }
                }
                if (y & 1) { ++x; }
                ++y;
            }
            if (!(startY & 1)) { --startX; }
            ++startY;
        }
        return out;
    }
    Result execute_single_healing(Events::Unit& actor, Events::Unit& target, const Events& events,
        s2ne::Random& random, ExecutionContext context, const AwardAdapter& award);
    s2ne::Result<HostileResult> execute_pipeline(Events::Unit& actor, std::vector<Events::Unit>& targets,
        const Events::Unit& center, const Events* events, const PromotionTransaction* equipment,
        s2ne::Random& random, ExecutionContext context, const AwardAdapter& award, Events::Unit* partner = nullptr,
        const Drain* drain = nullptr, std::vector<uint8_t>* army_inventory = nullptr, const SummonAdapter& summon = {});
    void resume();
    void cancel();
    void navigate(unsigned scan, bool change_group = true);
    bool mouse(int x, int y, bool click);
    s2ne::Result<bool> try_draw(std::vector<uint8_t>& image, const Events::Unit& unit, FullWidthCanvas* canvas = nullptr) const;
    void draw(std::vector<uint8_t>& image, const Events::Unit& unit, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
