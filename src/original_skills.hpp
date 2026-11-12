#pragma once
#include "native_ability_buffs.hpp"
#include "native_damage_records.hpp"
#include "native_effect55.hpp"
#include "native_hostile_status.hpp"
#include "native_sleep2.hpp"
#include "native_special_status.hpp"
#include "native_timed_buffs.hpp"
#include "original_combat.hpp"
#include "original_drain.hpp"
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_promotion_transaction.hpp"
#include "original_sprite.hpp"
#include <functional>
#include <optional>

namespace s2ne
{
// 464C0C/464C83 -> 469F04. Selection requests are not execution results.
class OriginalSkills
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
        int word(size_t offset) const { return int(int16_t(unsigned(raw.at(offset)) | (unsigned(raw.at(offset + 1)) << 8))); }
        std::vector<uint8_t> name() const
        {
            std::vector<uint8_t> out;
            for (size_t i = 2; i < 13 && raw[i]; ++i)
            {
                out.push_back(raw[i]);
            }
            return out;
        }
        unsigned effect() const { return raw[19]; }
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
        bool debug{};
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
    using AwardAdapter = std::function<bool(OriginalEvents::Unit&, original_combat::Random&, int, int, unsigned)>;
    using SummonAdapter = std::function<bool(const OriginalEvents::Unit&, unsigned, original_combat::Random&)>;
    static int summon_level(const OriginalEvents::Unit& caster)
    {
        return std::clamp(int(int16_t(caster.sp)) / 20 + 1, 1, 10);
    }

private:
    OriginalMenu font_;
    std::vector<uint8_t> bin_, pal_, cel_;
    std::vector<Group> groups_;
    size_t header_{};
    int ap_base_{}, ap_repeat_{};
    std::array<int, 256> config_{};
    unsigned owner_id_{}, owner_appearance_{};
    Context context_;
    std::pair<int, int> area_camera_{};
    Page page_{ Page::closed };
    size_t group_{}, skill_{};
    std::optional<Request> request_;
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    size_t entry(unsigned id) const { return dword(pal_, 848 + (id - 1) * 8) - 11 - (id - 1); }
    std::vector<uint8_t> string(size_t p) const
    {
        std::vector<uint8_t> out;
        while (bin_.at(p))
        {
            out.push_back(bin_.at(p++));
        }
        return out;
    }
    s2ne::Result<bool> try_sprite(std::vector<uint8_t>& image, unsigned index, int left, int top) const
    {
        return draw_original_cel(image, cel_, pal_, index + 142, left, top, OriginalZeroRun::opaque);
    }

public:
    OriginalSkills() = default;
    static s2ne::Result<OriginalSkills> create(const std::filesystem::path& root, bool small_font = false)
    {
        OriginalSkills result;
        auto font = OriginalMenu::create(root);
        if (!font)
        {
            return s2ne::Result<OriginalSkills>::failure(font.error());
        }
        result.font_ = std::move(font).value();
        result.font_.small_font = small_font;
        auto resources = read_combat_resources(root);
        if (!resources)
        {
            return s2ne::Result<OriginalSkills>::failure(resources.error());
        }
        auto cel = read_resource(root / "SUPERSP.CEL");
        if (!cel)
        {
            return s2ne::Result<OriginalSkills>::failure(cel.error());
        }
        auto data = std::move(resources).value();
        result.bin_ = std::move(data.bin);
        result.pal_ = std::move(data.pal);
        result.cel_ = std::move(cel).value();
        ResourceReader p(result.pal_), b(result.bin_);
        unsigned address = p.number(856, 4);
        if (!p)
        {
            return s2ne::Result<OriginalSkills>::failure(p.error());
        }
        if (address < 12)
        {
            return s2ne::Result<OriginalSkills>::failure(make_error("Invalid skill header address"));
        }
        result.header_ = address - 12;
        size_t h = result.header_;
        if (!b.extent(h, 9))
        {
            return s2ne::Result<OriginalSkills>::failure(b.error());
        }
        unsigned count = result.bin_[h + 1], first = result.bin_[h + 2], second = result.bin_[h + 3], records = result.bin_[h + 7], extra = result.bin_[h + 8];
        if (count > 11 || first + second > 11 || first + second > count)
        {
            return s2ne::Result<OriginalSkills>::failure(make_error("Invalid original skill groups"));
        }
        size_t recordData = h + 9 + count * 2, names = recordData + size_t(records) * 37, nameData = names + size_t(count + extra) * 2;
        if (!b.extent(h + 9, size_t(count) * 2 + size_t(records) * 37 + size_t(count + extra) * 2))
        {
            return s2ne::Result<OriginalSkills>::failure(b.error());
        }
        for (unsigned group = 0; group < count; ++group)
        {
            if (unsigned(result.bin_[h + 9 + group * 2]) + result.bin_[h + 10 + group * 2] > records)
            {
                return s2ne::Result<OriginalSkills>::failure(make_error("Invalid skill record range"));
            }
            size_t text = nameData + b.number(names + group * 2);
            if (text >= result.bin_.size() || std::find(result.bin_.begin() + text, result.bin_.end(), 0) == result.bin_.end())
            {
                return s2ne::Result<OriginalSkills>::failure(make_error("Invalid skill group name"));
            }
        }
        for (size_t i = 0; i < result.config_.size(); ++i)
        {
            result.config_[i] = int(int8_t(result.bin_[data.config + i]));
        }
        result.ap_base_ = result.config_[0xEF];
        result.ap_repeat_ = result.config_[0xF3];
        if (result.config_[0x8F] <= 0 || result.config_[0x91] <= 0 || result.config_[13] + 3 <= 0 || result.config_[0xE3] <= 0)
        {
            return s2ne::Result<OriginalSkills>::failure(make_error("Invalid original self-heal configuration"));
        }
        return s2ne::Result<OriginalSkills>::success(std::move(result));
    }
    explicit OriginalSkills(const std::filesystem::path& root, bool small_font = false)
    {
        auto result = create(root, small_font);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    int action_cost(const OriginalEvents::Unit& unit) const { return ap_base_ + ((unit.status34 & 12) ? ap_repeat_ : 0); }
    bool entry_available(const OriginalEvents::Unit& unit, Context context) const
    {
        unsigned begin = context.second ? bin_.at(header_ + 2) : 0, end = context.second ? begin + bin_.at(header_ + 3) : bin_.at(header_ + 2);
        if (context.debug || context.status3d_40 || ((unit.action_status_runtime ? unit.action_status_runtime->at(0x3D) : unit.status3d) & 0x40) || (context.second ? context.suppress_second : context.suppress_first) || unit.action_points < action_cost(unit))
        {
            return false;
        }
        for (unsigned g = begin; g < end; ++g)
        {
            if (unit.learned.at(g))
            {
                return true;
            }
        }
        return false;
    }
    void open(const OriginalEvents::Unit& unit, Context context)
    {
        context_ = context;
        owner_id_ = unit.id;
        owner_appearance_ = unit.appearance;
        groups_.clear();
        request_.reset();
        group_ = skill_ = 0;
        page_ = Page::closed;
        if (!entry_available(unit, context))
        {
            return;
        }
        unsigned count = bin_.at(header_ + 1), begin = context.second ? bin_.at(header_ + 2) : 0, end = context.second ? begin + bin_.at(header_ + 3) : bin_.at(header_ + 2);
        size_t records = header_ + 9 + count * 2, names = records + bin_.at(header_ + 7) * 37, name_data = names + 2 * (count + bin_.at(header_ + 8));
        for (unsigned g = begin; g < end; ++g)
        {
            if (!unit.learned.at(g))
            {
                continue;
            }
            Group group{ g, string(name_data + word(bin_, names + g * 2)), {} };
            unsigned first = bin_.at(header_ + 9 + g * 2), length = bin_.at(header_ + 10 + g * 2);
            for (unsigned bit = 0; bit < length; ++bit)
            {
                if (bit >= 16 || !(unit.learned[g] & (1u << bit)))
                {
                    continue;
                }
                Record r{ first + bit + 1, g, bit, {} };
                if (r.id > bin_.at(header_ + 7))
                {
                    report_error(make_error("Invalid original skill ID"));
                    groups_.clear();
                    page_ = Page::closed;
                    return;
                }
                for (size_t n = 0; n < 37; ++n)
                {
                    r.raw[n] = bin_.at(records + (r.id - 1) * 37 + n);
                }
                group.records.push_back(r);
            }
            groups_.push_back(std::move(group));
        }
        if (!groups_.empty() && !groups_[0].records.empty())
        {
            page_ = Page::list;
        }
    }
    const std::vector<Group>& groups() const { return groups_; }
    Page page() const { return page_; }
    size_t selected_group() const { return group_; }
    size_t selected_skill() const { return skill_; }
    const Record& selected() const { return groups_.at(group_).records.at(skill_); }
    Failure resource_failure(const OriginalEvents::Unit& unit, const Record& record) const
    {
        int cost = record.word(13);
        unsigned flags = record.raw[33];
        if ((flags & 1) && unit.mp < ((unit.abilities & 0x20) ? cost / 2 : cost))
        {
            return Failure::mp;
        }
        if ((flags & 2) && unit.st < cost)
        {
            return Failure::st;
        }
        if ((flags & 4) && unit.hp <= cost)
        {
            return Failure::hp;
        }
        if (unit.action_points < action_cost(unit))
        {
            return Failure::action_points;
        }
        return Failure::none;
    }
    template <class Elevation>
    std::optional<size_t> cooperative_partner(const OriginalEvents::Unit& actor, const std::vector<OriginalEvents::Unit>& units, unsigned turn, Elevation elevation) const
    {
        const auto& record = selected();
        if (!(record.raw[32] & 7)) { return std::nullopt; }
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
    bool cooperative_pair(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& partner) const
    {
        if (actor.id == partner.id && actor.appearance == partner.appearance) { return false; }
        if (actor.hostile() != partner.hostile() || !(partner.status_flags & 0x80) || (partner.status_flags & 0x20) || partner.action_state != 0 || !(partner.learned[selected().group] & (1u << selected().bit))) { return false; }
        const int right = actor.x + int(actor.y & 1), left = right - 1;
        if ((partner.y != actor.y - 1 && partner.y != actor.y + 1) || (partner.x != left && partner.x != right)) { return false; }
        if (selected().raw[32] & 6)
        {
            if (actor.id < 1 || actor.id > 40 || partner.id < 1 || partner.id > 40) { return false; }
            if (actor.relationships[partner.id - 1] < 70 || partner.relationships[actor.id - 1] < 70) { return false; }
        }
        return true;
    }
    Failure select(const OriginalEvents::Unit& unit, const OriginalEvents::Unit* partner = nullptr, bool summon_world_ready = false)
    {
        if (page_ != Page::list)
        {
            return Failure::world_unresolved;
        }
        if (unit.id != owner_id_ || unit.appearance != owner_appearance_ || !(unit.learned.at(selected().group) & (1u << selected().bit)))
        {
            return Failure::stale_unit;
        }
        if (!entry_available(unit, context_))
        {
            return Failure::world_unresolved;
        }
        if (auto failure = resource_failure(unit, selected()); failure != Failure::none)
        {
            return failure;
        }
        if (selected().raw[32] & 7)
        {
            if (!partner || !cooperative_pair(unit, *partner)) { return Failure::partner_unresolved; }
            if (auto failure = resource_failure(*partner, selected()); failure != Failure::none) { return failure; }
        }
        // 21..31 require 44CB0E world-state checks before leaving the list.
        if (selected().effect() >= 21 && selected().effect() <= 31 && (!summon_world_ready || !summon_supported()))
        {
            return Failure::world_unresolved;
        }
        request_ = Request{ unit.id, unit.appearance, selected().id, selected().group, selected().bit, context_.second };
        if (partner)
        {
            request_->partner_id = partner->id;
            request_->partner_appearance = partner->appearance;
        }
        page_ = Page::requested;
        return Failure::none;
    }
    const std::optional<Request>& request() const { return request_; }
    s2ne::Result<unsigned> try_initial_payment_state(unsigned id) const
    {
        size_t p = entry(10);
        if (!id || id > word(bin_, p))
        {
            return s2ne::Result<unsigned>::failure(make_error("Skill payment state"));
        }
        return s2ne::Result<unsigned>::success(bin_.at(p + 2 + word(bin_, p) + id - 1));
    }
    unsigned initial_payment_state(unsigned id) const { return try_initial_payment_state(id).value(); }
    bool effect_supported(const Record& record) const
    {
        // Exact installed healing records only; other effect18 formulas remain unavailable.
        static constexpr std::array<uint8_t, 24> tail{ 25, 0, 90, 0, 0, 1, 18, 41, 120, 0, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 66, 0, 0, 0 };
        static constexpr std::array<uint8_t, 24> heal{ 8, 0, 0, 0, 5, 1, 18, 1, 60, 0, 100, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 };
        return (record.id == 88 && record.group == 8 && record.bit == 5 && std::equal(tail.begin(), tail.end(), record.raw.begin() + 13))
            || (record.id == 1 && record.group == 0 && record.bit == 0 && std::equal(heal.begin(), heal.end(), record.raw.begin() + 13));
    }
    bool executable_available(const OriginalEvents::Unit& unit, Context context, bool award_bridge_ready) const
    {
        if (!award_bridge_ready || !entry_available(unit, context)) { return false; }
        auto probe = *this;
        probe.open(unit, context);
        for (const auto& g : probe.groups_)
        {
            for (const auto& r : g.records)
            {
                probe.group_ = size_t(&g - probe.groups_.data());
                probe.skill_ = size_t(&r - g.records.data());
                if (probe.pipeline_supported()) { return true; }
            }
        }
        return false;
    }
    Result execute(OriginalEvents::Unit&) const { return { false, Failure::unsupported_execution }; }
    Result execute(OriginalEvents::Unit& unit, const OriginalEvents::Unit& target, const OriginalEvents& events,
        original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        if (&unit != &target || selected().id != 88 || (context.status3c & 255) != unit.status3c) { return { false, Failure::world_unresolved }; }
        if (!request_ || unit.id != request_->unit_id || unit.appearance != request_->appearance || !(unit.learned[request_->group] & (1u << request_->bit))) { return { false, Failure::stale_unit }; }
        if (auto failure = resource_failure(unit, selected()); failure != Failure::none) { return { false, failure }; }
        return execute_single_healing(unit, unit, events, random, context, award);
    }
    bool healing1_supported() const { return page_ != Page::closed && selected().id == 1 && effect_supported(selected()); }
    bool healing1_in_range(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target) const
    {
        int dx = 2 * (target.x - actor.x) + int(target.y & 1) - int(actor.y & 1), dy = target.y - actor.y;
        return healing1_supported() && actor.hostile() == target.hostile() && target.hp > 0 && (target.status_flags & 0x80) && !(target.status_flags & 0x20) && std::max(std::abs(dx), std::abs(dy)) <= 5;
    }
    Result execute_healing1(OriginalEvents::Unit& actor, OriginalEvents::Unit& target, const OriginalEvents& events,
        original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        if (!healing1_supported()) { return { false, Failure::unsupported_execution }; }
        return execute_single_healing(actor, target, events, random, context, award);
    }
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
        std::vector<OriginalDrain::Packet> drain_targets;
    };
    bool sword94_supported() const { return page_ != Page::closed && selected().id == 94 && selected().group == 9 && selected().bit == 0 && selected().effect() == 1 && selected().raw[20] == 44 && selected().raw[33] == 194; }
    bool damage_supported() const
    {
        if (page_ == Page::closed) { return false; }
        if (!NativeDamageRecords::ordinary_sword(selected().id)) { return false; }
        const auto* original = NativeDamageRecords::find(selected().id);
        return original && selected().raw == original->raw;
    }
    bool drain_supported() const
    {
        if (page_ == Page::closed) { return false; }
        constexpr std::array<uint8_t, 37> blood{ 2, 8, 167, 108, 166, 229, 164, 106, 170, 107, 0, 0, 0, 45, 0, 180, 0, 1, 1, 42, 9, 75, 0, 70, 0, 0, 0, 2, 0, 0, 0, 2, 0, 1, 0, 0, 0 };
        constexpr std::array<uint8_t, 37> puppet{ 4, 8, 179, 200, 192, 119, 166, 229, 169, 71, 0, 0, 0, 25, 0, 180, 0, 3, 1, 41, 23, 1, 0, 60, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0 };
        return (selected().id == 18 && selected().raw == blood) || (selected().id == 40 && selected().raw == puppet);
    }
    bool fissure29_supported() const
    {
        constexpr std::array<uint8_t, 37> record{ 3, 9, 181, 245, 166, 97, 179, 78, 0, 0, 0, 0, 0, 24, 0, 208, 0, 2, 1, 3, 0, 0, 0, 20, 0, 0, 0, 0, 0, 2, 0, 2, 0, 1, 0, 0, 0 };
        return page_ != Page::closed && selected().id == 29 && selected().raw == record;
    }
    bool transfer51_supported() const
    {
        constexpr std::array<uint8_t, 37> record{ 5, 9, 182, 199, 176, 101, 179, 78, 0, 0, 0, 0, 0, 25, 0, 230, 0, 2, 1, 4, 28, 5, 0, 100, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0 };
        return page_ != Page::closed && selected().id == 51 && selected().raw == record;
    }
    bool summon_supported() const
    {
        if (page_ == Page::closed || selected().id < 64 || selected().id > 74) { return false; }
        constexpr unsigned costs[]{ 32, 32, 32, 32, 50, 50, 60, 60, 70, 70, 80 };
        constexpr unsigned thresholds[]{ 0, 25, 40, 60, 70, 70, 70, 70, 70, 70, 70 };
        const unsigned slot = selected().id - 64;
        const auto& r = selected();
        return r.group == 6 && r.bit == slot && r.raw[0] == 7 && r.raw[1] == slot + 1
            && r.word(13) == int(costs[slot]) && r.word(15) == int(thresholds[slot]) && !r.raw[17] && r.raw[18] == 1
            && r.effect() == slot + 21 && !r.raw[20] && !r.word(21) && r.word(23) == 100 && r.word(25) == 99
            && !r.word(27) && !r.word(29) && !r.raw[31] && r.raw[32] == (slot >= 4 ? 128 : 0)
            && r.raw[33] == 1 && r.raw[34] == 26 - slot && r.raw[35] == 5 && !r.raw[36];
    }
    bool curse33_supported() const { return page_ != Page::closed && selected().id == 33 && selected().group == 3 && selected().bit == 0 && selected().effect() == 55 && selected().raw[31] == 2; }
    bool sleep2_supported() const
    {
        constexpr std::array<uint8_t, 24> tail{ 18, 0, 10, 0, 3, 2, 61, 0, 0, 0, 40, 0, 2, 0, 0, 0, 2, 0, 2, 0, 1, 38, 38, 0 };
        return page_ != Page::closed && selected().id == 2 && selected().group == 0 && selected().bit == 1 && std::equal(tail.begin(), tail.end(), selected().raw.begin() + 13);
    }
    bool hostile_stat_supported() const
    {
        if (page_ == Page::closed) { return false; }
        constexpr std::array<unsigned, 4> ids{ 17, 25, 52, 61 };
        constexpr std::array<std::array<uint8_t, 24>, 4> tails{ { { 25, 0, 128, 0, 4, 1, 16, 0, 0, 0, 70, 0, 5, 0, 0, 0, 2, 0, 2, 0, 1, 7, 11, 0 },
            { 25, 0, 50, 0, 3, 2, 46, 16, 10, 0, 65, 0, 3, 0, 0, 0, 128, 0, 2, 0, 1, 12, 9, 0 },
            { 45, 0, 24, 1, 4, 1, 15, 29, 40, 0, 60, 0, 4, 0, 0, 0, 2, 0, 2, 0, 1, 33, 100, 0 },
            { 40, 0, 230, 0, 0, 7, 46, 16, 12, 0, 50, 0, 3, 0, 0, 0, 129, 0, 2, 0, 1, 44, 29, 0 } } };
        constexpr std::array<unsigned, 4> groups{ 1, 2, 4, 5 }, bits{ 6, 4, 9, 8 };
        for (size_t i = 0; i < ids.size(); ++i)
        {
            if (selected().id == ids[i] && selected().group == groups[i] && selected().bit == bits[i] && std::equal(tails[i].begin(), tails[i].end(), selected().raw.begin() + 13)) { return true; }
        }
        return false;
    }
    bool special_status_supported() const
    {
        if (page_ == Page::closed) { return false; }
        constexpr std::array<unsigned, 7> ids{ 8, 12, 35, 48, 60, 39, 56 };
        constexpr std::array<unsigned, 7> groups{ 0, 1, 3, 4, 5, 3, 5 }, bits{ 7, 1, 2, 5, 7, 6, 3 };
        constexpr std::array<std::array<uint8_t, 24>, 7> tails{ { { 20, 0, 170, 0, 4, 1, 20, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 37, 45, 0 },
            { 13, 0, 10, 0, 3, 1, 56, 0, 0, 0, 40, 0, 210, 0, 0, 0, 2, 0, 2, 0, 1, 8, 10, 0 },
            { 20, 0, 20, 0, 4, 1, 40, 0, 0, 0, 70, 0, 3, 0, 0, 0, 2, 0, 3, 0, 1, 15, 5, 0 },
            { 30, 0, 90, 0, 5, 3, 47, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 42, 31, 0 },
            { 30, 0, 180, 0, 3, 1, 5, 0, 0, 0, 35, 0, 0, 0, 0, 0, 2, 0, 2, 0, 1, 50, 16, 0 },
            { 25, 0, 128, 0, 4, 2, 58, 22, 20, 0, 70, 0, 3, 0, 0, 0, 128, 0, 3, 0, 1, 0, 0, 0 },
            { 20, 0, 32, 0, 5, 1, 74, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 41, 32, 0 } } };
        for (size_t i = 0; i < ids.size(); ++i)
        {
            if (selected().id == ids[i] && selected().group == groups[i] && selected().bit == bits[i] && std::equal(tails[i].begin(), tails[i].end(), selected().raw.begin() + 13)) { return true; }
        }
        return false;
    }
    bool hostile_status_supported() const
    {
        if (hostile_stat_supported()) { return true; }
        if (page_ == Page::closed) { return false; }
        constexpr std::array<std::array<uint8_t, 24>, 7> tails{ { { 15, 0, 20, 0, 4, 1, 69, 0, 0, 0, 65, 0, 4, 0, 0, 0, 0, 0, 2, 0, 1, 11, 9, 0 },
            { 30, 0, 56, 0, 4, 1, 67, 0, 0, 0, 60, 0, 4, 0, 0, 0, 2, 0, 2, 0, 1, 5, 13, 0 },
            { 15, 0, 0, 0, 4, 2, 8, 0, 0, 0, 40, 0, 3, 0, 8, 0, 34, 0, 2, 0, 1, 40, 34, 0 },
            { 13, 0, 10, 0, 3, 1, 43, 0, 0, 0, 50, 0, 3, 0, 0, 0, 2, 0, 2, 0, 1, 14, 6, 0 },
            { 20, 0, 90, 0, 5, 1, 17, 0, 0, 0, 60, 0, 4, 0, 0, 0, 2, 0, 2, 0, 1, 6, 12, 0 },
            { 20, 0, 10, 0, 5, 1, 13, 0, 0, 0, 45, 0, 4, 0, 0, 0, 34, 0, 2, 0, 1, 35, 80, 0 }, { 15, 0, 20, 0, 3, 1, 43, 0, 0, 0, 40, 0, 3, 0, 0, 0, 0, 0, 2, 0, 194, 10, 10, 0 } } };
        const auto& r = selected();
        unsigned i = 5;
        switch (r.id)
        {
        case 13: i = 0; break;
        case 15: i = 1; break;
        case 53: i = 2; break;
        case 34: i = 3; break;
        case 38: i = 4; break;
        case 96: i = 6; break;
        default: break;
        }
        return ((r.id == 13 && r.group == 1 && r.bit == 2) || (r.id == 15 && r.group == 1 && r.bit == 4) || (r.id == 53 && r.group == 5 && r.bit == 0) || (r.id == 34 && r.group == 3 && r.bit == 1) || (r.id == 38 && r.group == 3 && r.bit == 5) || (r.id == 54 && r.group == 5 && r.bit == 1) || (r.id == 96 && r.group == 9 && r.bit == 2))
            && std::equal(tails[i].begin(), tails[i].end(), r.raw.begin() + 13);
    }
    bool blessing3_supported() const
    {
        constexpr std::array<uint8_t, 24> tail{ 12, 0, 20, 0, 4, 2, 19, 2, 10, 0, 100, 0, 4, 0, 0, 0, 0, 0, 1, 0, 1, 54, 12, 0 };
        return page_ != Page::closed && selected().id == 3 && selected().group == 0 && selected().bit == 2 && std::equal(tail.begin(), tail.end(), selected().raw.begin() + 13);
    }
    bool strengthening4_supported() const
    {
        constexpr std::array<uint8_t, 24> tail{ 20, 0, 32, 0, 3, 2, 9, 3, 15, 0, 100, 0, 3, 0, 0, 0, 0, 0, 1, 0, 1, 55, 10, 0 };
        return page_ != Page::closed && selected().id == 4 && selected().group == 0 && selected().bit == 3 && std::equal(tail.begin(), tail.end(), selected().raw.begin() + 13);
    }
    bool protection5_supported() const
    {
        constexpr std::array<uint8_t, 24> tail{ 30, 0, 54, 0, 3, 1, 44, 0, 150, 0, 100, 0, 2, 0, 0, 0, 0, 0, 1, 0, 1, 39, 35, 0 };
        return page_ != Page::closed && selected().id == 5 && selected().group == 0 && selected().bit == 4 && std::equal(tail.begin(), tail.end(), selected().raw.begin() + 13);
    }
    bool restoration_supported() const
    {
        if (page_ == Page::closed || selected().group != 0) { return false; }
        static constexpr std::array<std::array<uint8_t, 24>, 3> tails{ { { 32, 0, 85, 0, 3, 3, 18, 4, 120, 0, 100, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 },
            { 40, 0, 214, 0, 3, 2, 18, 6, 244, 1, 100, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 },
            { 55, 0, 4, 1, 5, 1, 65, 0, 180, 0, 100, 0, 2, 0, 0, 0, 0, 0, 1, 0, 1, 36, 60, 0 } } };
        const unsigned id = selected().id;
        unsigned index = 2;
        if (id == 6) { index = 0; }
        else if (id == 9) { index = 1; }
        return (id == 6 || id == 9 || id == 10) && selected().bit == id - 1 && std::equal(tails[index].begin(), tails[index].end(), selected().raw.begin() + 13);
    }
    bool area_healing_supported() const { return (restoration_supported() && selected().effect() == 18) || (cooperative_supported() && selected().effect() == 90); }
    bool timed_buff_supported() const
    {
        if (page_ == Page::closed) { return false; }
        const auto* o = NativeTimedBuffs::find(selected().id);
        return o && selected().group == o->group && selected().bit == o->bit && std::equal(o->tail.begin(), o->tail.end(), selected().raw.begin() + 13);
    }
    bool cooperative_supported() const
    {
        if (page_ == Page::closed) { return false; }
        const auto* o = NativeAbilityBuffs::find(selected().id);
        return o && (o->id == 76 || o->id == 78 || o->id == 79) && selected().group == o->group && selected().bit == o->bit && std::equal(o->tail.begin(), o->tail.end(), selected().raw.begin() + 13);
    }
    bool ability_buff_supported() const
    {
        if (page_ == Page::closed) { return false; }
        const auto* o = NativeAbilityBuffs::find(selected().id);
        if (selected().id == 47 || selected().id == 76 || selected().id == 78 || selected().id == 79) { return false; }
        return o && selected().group == o->group && selected().bit == o->bit && std::equal(o->tail.begin(), o->tail.end(), selected().raw.begin() + 13);
    }
    bool invisibility47_supported() const
    {
        if (page_ == Page::closed) { return false; }
        const auto& oracle = *NativeAbilityBuffs::find(47);
        return selected().id == 47 && selected().group == oracle.group && selected().bit == oracle.bit && std::equal(oracle.tail.begin(), oracle.tail.end(), selected().raw.begin() + 13);
    }
    bool reveal50_supported() const
    {
        constexpr std::array<uint8_t, 24> tail{ 25, 0, 180, 0, 0, 7, 45, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 31, 150, 0 };
        return page_ != Page::closed && selected().id == 50 && selected().group == 4 && selected().bit == 7 && std::equal(tail.begin(), tail.end(), selected().raw.begin() + 13);
    }
    bool spirit45_supported() const
    {
        constexpr std::array<uint8_t, 24> tail{ 15, 0, 20, 0, 7, 1, 35, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0 };
        return page_ != Page::closed && selected().id == 45 && selected().group == 4 && selected().bit == 2 && std::equal(tail.begin(), tail.end(), selected().raw.begin() + 13);
    }
    bool friendly_status_supported() const { return invisibility47_supported() || cooperative_supported() || ability_buff_supported() || timed_buff_supported() || blessing3_supported() || strengthening4_supported() || protection5_supported() || restoration_supported(); }
    void pay_staged(OriginalEvents::Unit& actor, ExecutionContext context, uint32_t payment_abilities) const
    {
        const unsigned mask = request_->second ? 8 : 4;
        actor.action_points -= ap_base_ + ((actor.status34 & mask) ? ap_repeat_ : 0);
        actor.status34 |= uint8_t(mask);
        actor.carry_points = std::min(actor.carry_points, actor.action_points);
        if (context.skill_payment_state == 2) { return; }
        const int cost = selected().word(13);
        if (selected().raw[33] & 1) { actor.mp -= (payment_abilities & 0x20) ? cost / 2 : cost; }
        if (selected().raw[33] & 2) { actor.st -= cost; }
        if (selected().raw[33] & 4) { actor.hp = std::max(1, actor.hp - cost); }
    }
    bool status_area_supported() const { return fissure29_supported() || spirit45_supported() || reveal50_supported() || damage_supported() || drain_supported() || special_status_supported() || sleep2_supported() || hostile_status_supported() || friendly_status_supported(); }
    bool status_area_target(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target) const
    {
        return status_area_supported() && (!damage_supported() || target.id != actor.id || target.appearance != actor.appearance) && (selected().raw[31] == 3 || selected().raw[31] == 0 || ((target.hostile() == actor.hostile()) == (selected().raw[31] == 1)));
    }
    bool sleep2_center_in_range(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& center) const
    {
        int dx = 2 * (center.x - actor.x) + int(center.y & 1) - int(actor.y & 1), dy = center.y - actor.y;
        return status_area_supported() && ((damage_supported() && selected().raw[18] == 7) || std::max(std::abs(dx), std::abs(dy)) <= int(selected().raw[17]));
    }
    void set_area_camera(std::pair<int, int> camera) { area_camera_ = camera; }
    std::function<int(int, int)> area_elevation_;
    std::function<bool(int, int)> area_valid_;
    void set_area_elevation(std::function<int(int, int)> elevation) { area_elevation_ = std::move(elevation); }
    void set_area_valid(std::function<bool(int, int)> valid) { area_valid_ = std::move(valid); }
    std::vector<std::pair<int, int>> radial_cells(const OriginalEvents::Unit& center, bool preserve_ray_slots = false) const
    {
        std::vector<std::pair<int, int>> cells{ { center.x, center.y } };
        const unsigned mode = selected().raw[18];
        const int radius = mode == 6 ? 6 : selected().raw[17];
        if (mode == 5 || mode == 6)
        {
            std::array<std::pair<int, int>, 4> rays;
            rays.fill({ center.x, center.y });
            std::array<bool, 4> active{ true, true, true, true };
            for (int n = 0; n < radius; ++n)
            {
                for (unsigned d = 0; d < 4; ++d)
                {
                    if (!active[d])
                    {
                        if (preserve_ray_slots) { cells.emplace_back(-1, -1); }
                        continue;
                    }
                    auto& [x, y] = rays[d];
                    if (d == 0 || d == 3)
                    {
                        if (y & 1) { ++x; }
                    }
                    else if (!(y & 1)) { --x; }
                    y += d < 2 ? -1 : 1;
                    if (area_valid_ && !area_valid_(x, y))
                    {
                        active[d] = false;
                        if (preserve_ray_slots) { cells.emplace_back(-1, -1); }
                        continue;
                    }
                    if (area_elevation_)
                    {
                        int difference = area_elevation_(center.x, center.y) - area_elevation_(x, y);
                        if (difference < -2 || difference > 3)
                        {
                            active[d] = false;
                            if (preserve_ray_slots) { cells.emplace_back(-1, -1); }
                            continue;
                        }
                    }
                    cells.emplace_back(x, y);
                }
            }
            return cells;
        }
        // 403916/403A23: four wedges, axial cell then the two flanks of each layer.
        for (unsigned d = 0; d < 4; ++d)
        {
            int x = center.x, y = center.y;
            for (int n = 0; n < radius; ++n)
            {
                if (d == 0 || d == 3)
                {
                    if (!(y & 1)) { --x; }
                }
                else if (y & 1) { ++x; }
                y += d < 2 ? 1 : -1;
                bool stop = false;
                if (area_valid_) { stop = !area_valid_(x, y); }
                if (area_elevation_) { stop |= std::abs(area_elevation_(center.x, center.y) - area_elevation_(x, y)) > 10; }
                if (!stop) { cells.emplace_back(x, y); }
                int rightX = x, rightY = y, leftX = x, leftY = y;
                for (int k = 0; k < n; ++k)
                {
                    if (d < 2)
                    {
                        if (rightY & 1) { ++rightX; }
                    }
                    else if (!(rightY & 1)) { --rightX; }
                    rightY += (d == 0 || d == 3) ? 1 : -1;
                    if (d < 2)
                    {
                        if (!(leftY & 1)) { --leftX; }
                    }
                    else if (leftY & 1) { ++leftX; }
                    leftY += (d == 0 || d == 3) ? -1 : 1;
                    if ((!area_valid_ || area_valid_(rightX, rightY)) && (!area_elevation_ || std::abs(area_elevation_(center.x, center.y) - area_elevation_(rightX, rightY)) <= 10)) { cells.emplace_back(rightX, rightY); }
                    if ((!area_valid_ || area_valid_(leftX, leftY)) && (!area_elevation_ || std::abs(area_elevation_(center.x, center.y) - area_elevation_(leftX, leftY)) <= 10)) { cells.emplace_back(leftX, leftY); }
                }
                if (stop) { break; }
            }
        }
        return cells;
    }
    bool viewport_area_cell(int x, int y) const
    {
        // 4028DC -> 403717: mode7 enumerates the original camera's 19x36 staggered viewport, not radius7.
        const int column = area_camera_.first / 32, row = area_camera_.second / 16;
        const bool same = (column & 1) == (row & 1);
        const int leftEven = column / 2 + (same ? 0 : (column & 1));
        int oddOffset = row & 1;
        if (!same) { oddOffset = (column & 1) - ((row & 1) ? 0 : 1); }
        const int leftOdd = column / 2 + oddOffset;
        const int dy = y - row;
        if (dy < 0 || dy >= 36) { return false; }
        const int left = (dy & 1) ? leftOdd : leftEven;
        int count = same ? 10 : 9;
        if (dy & 1) { count = same ? 9 : 10; }
        return x >= left && x < left + count;
    }
    bool sleep2_area(const OriginalEvents::Unit& center, const OriginalEvents::Unit& target) const
    {
        if (selected().raw[18] == 7) { return viewport_area_cell(target.x, target.y); }
        if (selected().raw[18] == 5 || selected().raw[18] == 6 || selected().raw[18] == 8)
        {
            const auto cells = radial_cells(center);
            return std::find(cells.begin(), cells.end(), std::pair{ target.x, target.y }) != cells.end();
        }
        // 402701: modes1/2/3 enumerate 1/3/5 staggered rows.
        const int side = 2 * int(selected().raw[18]) - 1;
        int startY = center.y - side + 1, startX = center.x;
        for (int row = 0; row < side; ++row)
        {
            int x = startX, y = startY;
            for (int col = 0; col < side; ++col)
            {
                if (x == target.x && y == target.y) { return true; }
                if (y & 1) { ++x; }
                ++y;
            }
            if (!(startY & 1)) { --startX; }
            ++startY;
        }
        return false;
    }
    s2ne::Result<HostileResult> execute_sleep2(OriginalEvents::Unit& actor, std::vector<OriginalEvents::Unit>& targets,
        const OriginalEvents::Unit& center, original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        return execute_pipeline(actor, targets, center, nullptr, nullptr, random, context, award);
    }
    s2ne::Result<HostileResult> execute_blessing3(OriginalEvents::Unit& actor, std::vector<OriginalEvents::Unit>& targets,
        const OriginalEvents::Unit& center, original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        return execute_pipeline(actor, targets, center, nullptr, nullptr, random, context, award);
    }
    s2ne::Result<HostileResult> execute_strengthening_status(OriginalEvents::Unit& actor, std::vector<OriginalEvents::Unit>& targets,
        const OriginalEvents::Unit& center, const OriginalEvents& events, original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        return execute_pipeline(actor, targets, center, &events, nullptr, random, context, award);
    }
    s2ne::Result<HostileResult> execute_restoration(OriginalEvents::Unit& actor, std::vector<OriginalEvents::Unit>& targets,
        const OriginalEvents::Unit& center, const OriginalEvents& events, original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        return execute_pipeline(actor, targets, center, &events, nullptr, random, context, award);
    }
    bool hostile_supported() const { return page_ != Page::closed && selected().id == 32 && selected().group == 2 && selected().bit == 11 && selected().effect() == 34; }
    // 401146 builds concentric diamond rings; record17 is the radius, not AP.
    bool hostile_in_range(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target) const
    {
        if (transfer51_supported())
        {
            const int dx = 2 * (target.x - actor.x) + int(target.y & 1) - int(actor.y & 1), dy = target.y - actor.y;
            return target.hp > 0 && (target.status_flags & 0x80) && !(target.status_flags & 0x20) && std::max(std::abs(dx), std::abs(dy)) <= 2;
        }
        if (reveal50_supported()) { return actor.id == target.id && actor.appearance == target.appearance; }
        if (spirit45_supported()) { return target.hp > 0 && (target.status_flags & 0x80) && !(target.status_flags & 0x20) && sleep2_center_in_range(actor, target); }
        if (damage_supported() && selected().raw[17] == 0 && selected().raw[18] != 7) { return actor.id == target.id && actor.appearance == target.appearance; }
        if (hostile_stat_supported() && selected().raw[18] == 7) { return status_area_target(actor, target) && target.hp > 0 && (target.status_flags & 0x80) && !(target.status_flags & 0x20) && sleep2_area(actor, target); }
        if (fissure29_supported() || damage_supported() || drain_supported() || special_status_supported() || friendly_status_supported()) { return status_area_target(actor, target) && target.hp > 0 && (target.status_flags & 0x80) && !(target.status_flags & 0x20) && sleep2_center_in_range(actor, target); }
        if ((!hostile_supported() && !sword94_supported() && !curse33_supported() && !sleep2_supported() && !hostile_status_supported()) || actor.hostile() == target.hostile() || target.hp <= 0 || !(target.status_flags & 0x80) || (target.status_flags & 0x20))
        {
            return false;
        }
        int dx = 2 * (target.x - actor.x) + int(target.y & 1) - int(actor.y & 1), dy = target.y - actor.y;
        return std::max(std::abs(dx), std::abs(dy)) <= int(selected().raw[17]) && (&actor != &target);
    }
    HostileResult execute_hostile(OriginalEvents::Unit& actor, OriginalEvents::Unit& target,
        original_combat::Random& random, ExecutionContext context, const OriginalPromotionTransaction& equipment, const AwardAdapter& award)
    {
        std::vector<OriginalEvents::Unit> targets{ target };
        auto packet = execute_pipeline(actor, targets, target, nullptr, &equipment, random, context, award);
        if (!packet)
        {
            HostileResult out;
            out.failure = Failure::world_unresolved;
            return out;
        }
        target = std::move(targets.front());
        return packet.value();
    }
    s2ne::Result<HostileResult> execute_curse33(OriginalEvents::Unit& actor, OriginalEvents::Unit& target,
        original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        std::vector<OriginalEvents::Unit> targets{ target };
        auto packet = execute_pipeline(actor, targets, target, nullptr, nullptr, random, context, award);
        if (packet) { target = std::move(targets.front()); }
        return packet;
    }
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
    Handler handler(bool summon_ready = false) const
    {
        if (page_ == Page::closed) { return Handler::unavailable; }
        if (damage_supported()) { return Handler::damage466082; }
        if (drain_supported()) { return Handler::drain467625; }
        if (fissure29_supported()) { return Handler::fissure467625; }
        if (transfer51_supported()) { return Handler::transfer4665D8; }
        if (summon_ready && summon_supported()) { return Handler::summon465E04; }
        if (reveal50_supported()) { return Handler::army4666C5; }
        if (spirit45_supported()) { return Handler::spirit46864F; }
        if (special_status_supported()) { return Handler::specialStatus; }
        if (effect_supported(selected()) || area_healing_supported()) { return Handler::healing465C14; }
        if (friendly_status_supported()) { return Handler::status4668F0; }
        if (sleep2_supported() || curse33_supported() || hostile_status_supported()) { return Handler::hostile467625; }
        if (hostile_supported()) { return Handler::equipment467625; }
        return Handler::unavailable;
    }
    bool pipeline_supported() const { return handler() != Handler::unavailable; }
    template <class ValidCell>
    std::vector<size_t> collect_targets(const OriginalEvents::Unit& actor, const OriginalEvents::Unit& center,
        const std::vector<OriginalEvents::Unit>& units, unsigned turn, ValidCell valid) const
    {
        std::vector<size_t> out;
        if (selected().raw[18] == 5 || selected().raw[18] == 6 || selected().raw[18] == 8)
        {
            for (auto [x, y] : radial_cells(center))
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
    Result execute_single_healing(OriginalEvents::Unit& actor, OriginalEvents::Unit& target, const OriginalEvents& events,
        original_combat::Random& random, ExecutionContext context, const AwardAdapter& award)
    {
        std::vector<OriginalEvents::Unit> targets{ target };
        auto packet = execute_pipeline(actor, targets, target, &events, nullptr, random, context, award);
        if (!packet) { return { false, Failure::world_unresolved }; }
        if (&actor != &target) { target = std::move(targets.front()); }
        return { true, Failure::none, packet.value().target_amounts.front(), packet.value().raw_xp, packet.value().proficiency };
    }
    s2ne::Result<HostileResult> execute_pipeline(OriginalEvents::Unit& actor, std::vector<OriginalEvents::Unit>& targets,
        const OriginalEvents::Unit& center, const OriginalEvents* events, const OriginalPromotionTransaction* equipment,
        original_combat::Random& random, ExecutionContext context, const AwardAdapter& award, OriginalEvents::Unit* partner = nullptr,
        const OriginalDrain* drain = nullptr, std::vector<uint8_t>* army_inventory = nullptr, const SummonAdapter& summon = {})
    {
        auto fail = []
        {
            return s2ne::Result<HostileResult>::failure(make_error("Original cast pipeline rejected request/target/resources/adapter"));
        };
        const auto kind = handler(bool(summon));
        if (kind == Handler::damage466082 || kind == Handler::unavailable || page_ != Page::requested || !request_ || request_->unit_id != actor.id || request_->appearance != actor.appearance || !(actor.learned[request_->group] & (1u << request_->bit)) || !entry_available(actor, context_) || resource_failure(actor, selected()) != Failure::none || !context.friendly_army || context.difficulty < 1 || context.difficulty > 5 || !award || (targets.empty() && kind != Handler::army4666C5 && kind != Handler::summon465E04)) { return fail(); }
        if (context.conditions_mode && (kind == Handler::healing465C14 || kind == Handler::status4668F0)) { return fail(); }
        if ((kind == Handler::healing465C14 || strengthening4_supported() || ability_buff_supported()) && !events) { return fail(); }
        if (kind == Handler::equipment467625 && !equipment) { return fail(); }
        if (kind == Handler::drain467625 && (!drain || !events || !equipment || !army_inventory)) { return fail(); }
        if (kind == Handler::fissure467625 && (!events || !army_inventory || config_[11] + 3 <= 0)) { return fail(); }
        if (cooperative_supported() && (!partner || partner->id != request_->partner_id || partner->appearance != request_->partner_appearance || !cooperative_pair(actor, *partner) || resource_failure(*partner, selected()) != Failure::none)) { return fail(); }
        const unsigned group = request_->group;
        auto a = actor;
        auto p = partner ? *partner : actor;
        auto staged = targets;
        auto stream = random;
        auto inventory = army_inventory ? *army_inventory : std::vector<uint8_t>{};
        HostileResult out;
        out.reveal_army = kind == Handler::army4666C5;
        if (kind == Handler::summon465E04)
        {
            if (!targets.empty() || !summon(a, selected().id - 64, stream)) { return fail(); }
            out.raw_xp = 5;
        }
        int successes = 0;
        for (size_t i = 0; i < staged.size(); ++i)
        {
            if (kind == Handler::army4666C5)
            {
                out.target_hits.push_back(false);
                out.target_amounts.push_back(0);
                continue;
            }
            if (kind == Handler::spirit46864F)
            {
                if (staged.size() != 1 || !hostile_in_range(actor, staged[i])) { return fail(); }
                out.chance = 100;
                if (events && (selected().raw[29] & 1) && events->skill_target_class(staged[i], staged[i].status3c)) { out.chance = 0; }
                if ((selected().raw[29] & 2) && staged[i].runtime_role_selector == 1) { out.chance = 0; }
                if ((selected().raw[29] & 0x20) && staged[i].runtime_role_selector == 2) { out.chance = 0; }
                out.roll = stream.percent(context.seconds);
                out.hit = out.roll <= out.chance;
                out.target_hits.push_back(false);
                out.target_amounts.push_back(0);
                continue;
            }
            auto& t = staged[i];
            for (size_t n = 0; n < i; ++n)
            {
                if (staged[n].id == t.id && staged[n].appearance == t.appearance) { return fail(); }
            }
            if (t.hp <= 0 || !(t.status_flags & 0x80) || (t.status_flags & 0x20)) { return fail(); }
            if (status_area_supported())
            {
                if (!sleep2_center_in_range(actor, center) || !sleep2_area(center, t) || !status_area_target(actor, t)) { return fail(); }
            }
            else if (kind == Handler::healing465C14)
            {
                if (selected().id == 88)
                {
                    if (t.id != actor.id || t.appearance != actor.appearance || t.x != actor.x || t.y != actor.y || targets.size() != 1) { return fail(); }
                }
                else if (!healing1_in_range(actor, t)) { return fail(); }
            }
            else if (!hostile_in_range(actor, t) || targets.size() != 1) { return fail(); }
            if (kind == Handler::transfer4665D8)
            {
                // 4665D8 delegates relocation to45DB10 after selection; settlement stays here.
                if (!transfer51_supported() || targets.size() != 1) { return fail(); }
                out.target_hits.push_back(false);
                out.target_amounts.push_back(0);
            }
            else if (kind == Handler::fissure467625)
            {
                if (!t.action_status_runtime) { return fail(); }
                auto& runtime = *t.action_status_runtime;
                if ((unsigned(runtime[8]) | unsigned(runtime[9]) << 8) == 2)
                {
                    t.ai_role10 = 1;
                    runtime[0xA] = 1;
                    runtime[0xB] = runtime[0xC] = runtime[0xD] = 0;
                }
                OriginalDrain::Packet packet;
                if (t.runtime_role_selector != 1)
                {
                    packet.chance = OriginalSkill94Formula::hit_curve(a.st - t.st + 20 - 100);
                    packet.chance = packet.chance * (config_[10] + stream.bounded(config_[11] + 3, context.seconds)) / 10;
                    if (context.friendly_army && !context.conditions_mode) { packet.chance = packet.chance * (100 + config_[0xF3 + context.difficulty]) / 100; }
                    packet.chance = std::clamp(packet.chance, 1, 99);
                }
                packet.roll = stream.percent(context.seconds);
                packet.hit = packet.roll <= packet.chance;
                packet.snapshot = t.hp;
                if (packet.hit)
                {
                    t.status3e = runtime[0x3E] &= uint8_t(~2u);
                    if (!(context.difficulty == 2 && !a.hostile()))
                    {
                        if ((t.status_flags & 0x40) || ((t.status_flags & 0x10) && !events->army_status) || !OriginalDeathDispatch::continuation(*events, t.id).supported) { return fail(); }
                        packet.killed = true;
                        t.hp = 0;
                        t.status_flags |= 0x20;
                        runtime[0x33] = uint8_t(t.status_flags);
                        runtime[0x124] = runtime[0x125] = 0;
                        t.ending_losses = uint16_t(t.ending_losses + 1);
                        a.ending_kills = uint16_t(a.ending_kills + 1);
                        for (const auto item : t.initial_status)
                        {
                            if (!item) { continue; }
                            if (item > inventory.size()) { return fail(); }
                            inventory[item - 1] = uint8_t(std::min(99u, unsigned(inventory[item - 1]) + 1));
                        }
                        out.raw_xp = 1;
                    }
                }
                out.drain_targets.push_back(packet);
                out.chance = packet.chance;
                out.roll = packet.roll;
                out.hit |= packet.hit;
                out.target_hits.push_back(packet.hit);
                out.target_amounts.push_back(0);
            }
            else if (kind == Handler::drain467625)
            {
                OriginalSkill94Resolution::DamageContext modifiers;
                modifiers.difficulty = context.difficulty;
                modifiers.friendly_army = context.friendly_army;
                modifiers.conditions_mode = context.conditions_mode;
                auto packet = drain->resolve(a, t, *events, *equipment, inventory, stream, modifiers,
                    selected().id, context.seconds, OriginalDeathDispatch::continuation(*events, t.id).supported);
                if (!packet) { return fail(); }
                out.drain_targets.push_back(packet.value());
                out.chance = packet.value().chance;
                out.roll = packet.value().roll;
                out.hit |= packet.value().hit;
                out.target_hits.push_back(packet.value().hit);
                out.target_amounts.push_back(packet.value().damage);
                if (packet.value().killed) { out.raw_xp = 1; }
            }
            else if (kind == Handler::healing465C14)
            {
                if (t.hp > t.max_hp) { return fail(); }
                if (selected().effect() == 90)
                {
                    if (!t.action_status_runtime) { return fail(); }
                    auto& r = *t.action_status_runtime;
                    r[0x38] &= 0x74;
                    r[0x39] &= 0xB9;
                    r[0x3D] &= 0x0F;
                    r[0x3E] &= 0xF0;
                    r[0x3F] &= 0xFE;
                    auto rebuilt = original_combat::try_recalculate_status_stats(r);
                    if (!rebuilt) { return fail(); }
                    auto word = [&](size_t p)
                    {
                        return int(int16_t(unsigned(r[p]) | (unsigned(r[p + 1]) << 8)));
                    };
                    t.sp = word(0x12A);
                    t.max_action_points = word(0x12C);
                    t.attack = word(0x134);
                    t.defense = word(0x136);
                    t.tactics = word(0x138);
                    t.status39 = r[0x39];
                    t.status3c = r[0x3C];
                    t.status3d = r[0x3D];
                    t.status3e = r[0x3E];
                    t.status3f = r[0x3F];
                }
                bool blocked = (context.status3c & 0x10000) != 0;
                if (selected().id != 88)
                {
                    const unsigned status = t.action_status_runtime ? t.action_status_runtime->at(0x3E) : t.status3e;
                    blocked = (status & 1) != 0;
                }
                int amount = 0;
                if (!blocked)
                {
                    if (selected().raw[20] == 41)
                    {
                        const int fencing = std::max(1, actor.fencing);
                        amount = fencing * config_[0x8E] / config_[0x8F] - fencing * config_[0x90] / config_[0x91] + selected().word(21);
                    }
                    else if (selected().effect() == 90) { amount = selected().word(21); }
                    else
                    {
                        unsigned offset = 0x1F;
                        if (selected().raw[20] == 1) { offset = 0x12; }
                        else if (selected().raw[20] == 4) { offset = 0x19; }
                        if (config_[offset] <= 0) { return fail(); }
                        const int st = actor.st + (events->equipment_special(actor, 18) ? actor.st / 5 : 0);
                        amount = st * config_[offset - 1] / config_[offset] + selected().word(21);
                    }
                    if (events->equipment_special(t, 19)) { amount *= 2; }
                    amount = std::max(1, amount * (config_[12] + stream.bounded(config_[13] + 3, context.seconds)) / 10);
                }
                const bool feedback = t.hp < t.max_hp;
                int healed = 0;
                if (feedback)
                {
                    const int chance = blocked && selected().id != 88 ? 0 : selected().word(23);
                    if (stream.percent(context.seconds) <= chance) { healed = std::min(amount, t.max_hp - t.hp); }
                    t.hp += healed;
                    out.raw_xp += std::clamp(healed / config_[0xE3], 1, 60);
                    successes += healed > 0;
                }
                // Existing single-target animations also presented zero at full HP.
                out.target_hits.push_back(feedback || selected().id == 1 || selected().id == 88);
                out.target_amounts.push_back(healed);
                out.hit |= healed > 0;
            }
            else if (kind == Handler::status4668F0)
            {
                const bool strengthening = strengthening4_supported();
                if (!t.action_status_runtime)
                {
                    t.action_status_runtime.emplace();
                    auto& r = *t.action_status_runtime;
                    r[0x3C] = t.status3c;
                    r[0x3D] = t.status3d;
                    r[0x3E] = t.status3e;
                    r[0x3F] = t.status3f;
                }
                int power = selected().word(21);
                if (strengthening || (ability_buff_supported() && (selected().effect() == 2 || selected().effect() == 6 || selected().effect() == 10)))
                {
                    const int st = actor.st + (events->equipment_special(actor, 18) ? actor.st / 5 : 0);
                    const unsigned formula = selected().raw[20];
                    unsigned numerator = 0x6E;
                    if (strengthening) { numerator = 0x16; }
                    else if (formula == 13) { numerator = 0x38; }
                    else if (formula == 14) { numerator = 0x3A; }
                    if (config_[numerator + 1] <= 0 || (formula == 31 && config_[0x71] <= 0)) { return fail(); }
                    power += st * config_[numerator] / config_[numerator + 1];
                    if (formula == 31)
                    {
                        const int targetST = t.st + (events->equipment_special(t, 18) ? t.st / 5 : 0);
                        power -= targetST * config_[0x70] / config_[0x71];
                    }
                    const unsigned randomBase = 8;
                    if (config_[randomBase + 1] + 3 <= 0) { return fail(); }
                    power = std::max(1, power * (config_[randomBase] + stream.bounded(config_[randomBase + 1] + 3, context.seconds)) / 10);
                }
                out.chance = 99;
                out.roll = stream.percent(context.seconds);
                const bool hit = out.roll <= 99;
                out.hit |= hit;
                out.target_hits.push_back(false);
                out.target_amounts.push_back(0);
                if (hit)
                {
                    auto& r = *t.action_status_runtime;
                    bool changed = false;
                    if (cooperative_supported())
                    {
                        if (selected().id == 76)
                        {
                            auto applied = NativeAbilityBuffs::apply(t, 12, power, selected().word(25));
                            if (!applied) { return fail(); }
                            changed = applied.value();
                            for (unsigned mask : { 4u, 0x200u })
                            {
                                auto status = original_combat::try_apply_timed_status(r, mask, selected().word(25));
                                if (!status) { return fail(); }
                                changed |= status.value();
                            }
                            t.status3c = r[0x3C];
                            t.status3d = r[0x3D];
                        }
                        else if (selected().id == 79 && t.id != actor.id && t.id != p.id && t.action_state == 2 && t.persistent_role6 == 1)
                        {
                            t.action_state = 0;
                            t.status34 = 0;
                            t.carry_points = t.action_points;
                            if (t.action_points > config_[0xED]) { t.carry_points = config_[0xED]; }
                            if (t.action_points + t.max_action_points >= 110) { t.carry_points = 0; }
                            t.action_points = int(int16_t(t.max_action_points + t.carry_points));
                            r[0x2D] = 0;
                            r[0x34] = r[0x35] = r[0x36] = r[0x37] = 0;
                            r[0x12E] = uint8_t(t.action_points);
                            r[0x12F] = uint8_t(unsigned(t.action_points) >> 8);
                            r[0x130] = uint8_t(t.carry_points);
                            r[0x131] = uint8_t(unsigned(t.carry_points) >> 8);
                            changed = true;
                        }
                    }
                    else if (ability_buff_supported())
                    {
                        int duration = selected().word(25);
                        if (duration > 100 && duration <= 200) { duration = std::max(1, actor.st / (duration - 100)); }
                        else if (duration > 200 && duration < 255) { duration = std::max(1, (actor.st - t.st) / (duration - 200)); }
                        else if (duration >= 255) { duration = -1; }
                        if (context_.army_da_nonzero) { ++duration; }
                        auto applied = NativeAbilityBuffs::apply(t, selected().effect(), power, duration);
                        if (!applied) { return fail(); }
                        changed = applied.value();
                    }
                    else if (strengthening)
                    {
                        changed = !(r[0x38] & 0x20);
                        if (changed)
                        {
                            r[0x38] |= 0x20;
                            r[0x58] = uint8_t(power);
                            r[0x59] = uint8_t(unsigned(power) >> 8);
                            t.sp = int(int16_t(t.sp + power));
                            r[0x12A] = uint8_t(t.sp);
                            r[0x12B] = uint8_t(unsigned(t.sp) >> 8);
                        }
                        if (changed || (int8_t(r[0x57]) != -1 && int8_t(r[0x57]) < selected().word(25))) { r[0x57] = uint8_t(selected().word(25)); }
                    }
                    else
                    {
                        const auto* buff = timed_buff_supported() ? NativeTimedBuffs::find(selected().id) : nullptr;
                        uint32_t mask = 0x20;
                        if (buff) { mask = buff->mask; }
                        else if (invisibility47_supported()) { mask = 0x10; }
                        else if (selected().effect() == 19) { mask = 0x4000000; }
                        else if (selected().effect() == 65) { mask = 0x40; }
                        int duration = selected().word(25);
                        if (buff && duration > 100 && duration <= 200) { duration = std::max(1, actor.st / (duration - 100)); }
                        if (invisibility47_supported() && context_.army_da_nonzero) { ++duration; }
                        auto applied = original_combat::try_apply_timed_status(r, mask, duration);
                        if (!applied) { return fail(); }
                        changed = applied.value();
                        t.status3c = r[0x3C];
                        t.status3f = r[0x3F];
                    }
                    out.target_hits.back() = changed;
                    if (changed) { out.target_amounts.back() = power; }
                }
            }
            else if (kind == Handler::specialStatus)
            {
                int duration = selected().word(25);
                if (duration > 200 && duration < 255) { duration = std::max(1, (actor.st - t.st) / (duration - 200)); }
                auto packet = NativeSpecialStatus::resolve(t, actor, stream, config_, context.difficulty, context.seconds, context.conditions_mode, selected().id, selected().effect(), selected().raw[29], duration);
                if (!packet) { return fail(); }
                out.hit |= packet.value().hit;
                out.immune |= packet.value().immune;
                out.chance = packet.value().chance;
                out.roll = packet.value().roll;
                out.target_hits.push_back(packet.value().hit);
                out.target_amounts.push_back(0);
            }
            else if (kind == Handler::hostile467625)
            {
                const int duration = curse33_supported() ? std::max(1, (actor.st - t.st) / 10) : selected().word(25);
                int power = 0;
                if (hostile_stat_supported())
                {
                    if (!t.action_status_runtime) { return fail(); }
                    auto& r = *t.action_status_runtime;
                    auto put = [&](size_t p, int v)
                    {
                        r[p] = uint8_t(v);
                        r[p + 1] = uint8_t(unsigned(v) >> 8);
                    };
                    put(0x134, t.attack);
                    put(0x136, t.defense);
                    put(0x12C, t.max_action_points);
                    put(0x12E, t.action_points);
                    put(0x130, t.carry_points);
                    if (selected().effect() != 16)
                    {
                        // 46E058 default46E1D0 returns WORD21, discarding46E858's formula16/29 result.
                        power = selected().word(21);
                    }
                }
                auto packet = NativeHostileStatus::resolve(t, actor, stream, config_, context.difficulty, context.seconds, context.conditions_mode, selected().effect(), selected().raw[29], duration, power, hostile_stat_supported() ? selected().word(21) : 0, hostile_stat_supported() && (selected().raw[29] & 1) && events && events->movement_class(t) != 0);
                if (!packet) { return fail(); }
                const auto& p = packet.value();
                out.hit |= p.hit;
                out.immune |= p.immune;
                out.chance = p.chance;
                out.roll = p.roll;
                out.target_hits.push_back(p.hit);
                out.target_amounts.push_back(0);
            }
            else
            {
                std::vector<unsigned> slots;
                for (unsigned n = 1; n < 6; ++n)
                {
                    if (t.equipment[n]) { slots.push_back(n); }
                }
                int defense = 0;
                if (!slots.empty())
                {
                    out.slot = slots[size_t(stream.bounded(int(slots.size()), context.seconds))];
                    defense = equipment->item_defense(t.equipment[out.slot]);
                }
                int resistance = (actor.st - defense) / 2;
                resistance = resistance * (config_[10] + stream.bounded(config_[11] + 3, context.seconds)) / 10;
                if (!context.conditions_mode) { resistance = resistance * (100 + config_[0xF3 + context.difficulty]) / 100; }
                out.chance = std::clamp(5 + std::clamp(resistance, 1, 99), 1, 99);
                out.roll = stream.percent(context.seconds);
                out.hit = out.roll <= out.chance;
                unsigned status = t.action_status_runtime ? t.action_status_runtime->at(0x3E) : t.status3e;
                out.immune = (status & 0x10) || (t.abilities & 0x4000);
                if (out.hit)
                {
                    t.status3e = uint8_t(status & ~2u);
                    if (t.action_status_runtime) { t.action_status_runtime->at(0x3E) &= uint8_t(~2u); }
                    if (!out.immune && !slots.empty())
                    {
                        OriginalPromotionTransaction::State state{ t, std::vector<uint8_t>(equipment->item_count()), OriginalRandom({ 1, 0 }) };
                        out.removed = t.equipment[out.slot];
                        equipment->remove_equipment(state, out.slot, true);
                        t = std::move(state.unit);
                    }
                }
                out.target_hits.push_back(out.hit);
                out.target_amounts.push_back(0);
            }
            if (t.id == actor.id && t.appearance == actor.appearance) { a = t; }
            if (partner && t.id == p.id && t.appearance == p.appearance) { p = t; }
        }
        if (kind == Handler::summon465E04)
        {
            int learned = 0;
            for (unsigned bit = 0; bit < 11; ++bit)
            {
                if (actor.learned[6] & (1u << bit)) { ++learned; }
            }
            out.proficiency = (learned + 6 - int(context.difficulty)) * 5 + 20;
        }
        else if (kind == Handler::healing465C14) { out.proficiency = (successes + 6 - int(context.difficulty)) * 5 + 10; }
        else
        {
            if ((kind != Handler::drain467625 && kind != Handler::fissure467625) || out.raw_xp != 1) { out.raw_xp = 5; }
            out.proficiency = (selected().word(13) + 6 - int(context.difficulty)) * 5 / 2 + 10;
        }
        if (cooperative_supported())
        {
            const int half_xp = std::max(1, out.raw_xp / 2);
            // 46570C/4659F6 both add the unchanged +24h proficiency; only +1Ch/+20h are halved.
            if (!award(p, stream, half_xp, out.proficiency, group)) { return fail(); }
            out.raw_xp = half_xp;
        }
        if (!award(a, stream, out.raw_xp, out.proficiency, group)) { return fail(); }
        pay_staged(a, context, actor.abilities);
        if (cooperative_supported()) { pay_staged(p, context, partner->abilities); }
        for (auto& t : staged)
        {
            if (t.id == actor.id && t.appearance == actor.appearance) { t = a; }
            if (partner && t.id == p.id && t.appearance == p.appearance) { t = p; }
        }
        if (cooperative_supported()) { *partner = std::move(p); }
        actor = std::move(a);
        targets = std::move(staged);
        random = stream;
        if (army_inventory) { *army_inventory = std::move(inventory); }
        request_.reset();
        page_ = Page::closed;
        out.applied = true;
        return s2ne::Result<HostileResult>::success(out);
    }
    void resume()
    {
        if (page_ == Page::requested)
        {
            request_.reset();
            page_ = Page::list;
        }
    }
    void cancel()
    {
        request_.reset();
        page_ = Page::closed;
    }
    void navigate(unsigned scan)
    {
        if (page_ != Page::list)
        {
            return;
        }
        if (scan == 0x4B || scan == 0x4D)
        {
            group_ = scan == 0x4B ? (group_ + groups_.size() - 1) % groups_.size() : (group_ + 1) % groups_.size();
            skill_ = 0;
        }
        else if (scan == 0x48 || scan == 0x50)
        {
            size_t n = groups_[group_].records.size();
            if (n)
            {
                skill_ = scan == 0x48 ? (skill_ + n - 1) % n : (skill_ + 1) % n;
            }
        }
    }
    bool mouse(int x, int y, bool click)
    {
        if (page_ != Page::list)
        {
            return false;
        }
        if (x >= 26 && x <= 506 && y >= 394 && y <= 426)
        {
            size_t index = size_t(x - 26) / 34;
            if (index < groups_[group_].records.size())
            {
                bool same = skill_ == index;
                if (!click)
                {
                    skill_ = index;
                }
                return click && same;
            }
        }
        for (size_t g = 0; g < groups_.size(); ++g)
        {
            int cx = 508 + int(g % 3) * 45, cy = 394 + int(g / 3) * 28;
            if (x >= cx - 22 && x <= cx + 22 && y >= cy && y < cy + 16)
            {
                if (!click)
                {
                    group_ = g;
                    skill_ = 0;
                }
                return false;
            }
        }
        return false;
    }
    s2ne::Result<bool> try_draw(std::vector<uint8_t>& image) const
    {
        if (page_ == Page::closed)
        {
            return s2ne::Result<bool>::success(true);
        }
        if (image.size() != 640 * 480 * 4)
        {
            return s2ne::Result<bool>::failure(make_error("Skill menu requires 640x480 RGBA"));
        }
        auto output = image;
        auto sprite_result = try_sprite(output, 98, 0, 380);
        if (!sprite_result)
        {
            return sprite_result;
        }
        for (size_t g = 0; g < groups_.size(); ++g)
        {
            const auto& name = groups_[g].name;
            font_.draw_small(output, name, 508 + int(g % 3) * 45 - int(name.size()) * 4, 394 + int(g / 3) * 28, g == group_ ? 60 : 105, true);
        }
        for (size_t s = 0; s < groups_[group_].records.size(); ++s)
        {
            sprite_result = try_sprite(output, 121 + 2 * (groups_[group_].records[s].id - 1) + (s == skill_ ? 1 : 0), 26 + int(s) * 34, 394);
            if (!sprite_result)
            {
                return sprite_result;
            }
        }
        font_.draw_original_large(output, selected().name(), 148, 438, 168);
        image.swap(output);
        return s2ne::Result<bool>::success(true);
    }
    void draw(std::vector<uint8_t>& image) const
    {
        auto result = try_draw(image);
        if (!result)
        {
            report_error(result.error());
        }
    }
};
}    //namespace s2ne
