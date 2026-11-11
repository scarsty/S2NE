#pragma once
#include "original_ai.hpp"
#include "original_combat.hpp"
#include "original_combat_visual.hpp"
#include "original_disc.hpp"
#include "original_events.hpp"
#include "original_promotion_transaction.hpp"
#include "original_random.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <string>

namespace s2ne
{
// Original terrain/action-point arithmetic with basic contact melee; scored AI and abilities remain pending.
class Battle
{
public:
    struct Cell
    {
        int elevation{ }, ground{ }, object{ };
    };
    struct Point
    {
        int x{ }, y{ };
        bool operator==(const Point&) const = default;
    };

private:
    unsigned width_{ }, height_{ };
    std::vector<Cell> cells_;
    std::vector<unsigned> ground_attributes_, object_attributes_;
    std::vector<Point> teleport_pads_;
    std::function<bool(unsigned, Point)> teleport_presenter_;
    size_t teleport_cursor_{ };
    std::optional<Point> teleport_destination_;
    bool teleport_pending_{ };
    std::vector<uint8_t> movement_;
    std::vector<Point> sealed_cells_;
    std::array<int, 12> config_{ };
    unsigned turn_ = 1;
    bool hostile_side_ = false;
    std::vector<uint8_t> effect_palette_;
    std::array<std::vector<uint8_t>, 3> movement_effects_;
    std::array<std::vector<uint8_t>, 2> selection_frames_;
    std::vector<uint8_t> ground_cursor_;
    struct ProjectileSprite
    {
        unsigned width{ }, height{ };
        std::vector<uint8_t> pixels;
    };
    std::array<ProjectileSprite, 16> projectile59_;
    std::array<ProjectileSprite, 5> finish_directions_;
    std::optional<OriginalEvents::EffectFlight> projectile_;
    unsigned projectile_elapsed_{ };

public:
    bool projectile_active() const { return projectile_.has_value(); }

private:
    std::array<std::vector<uint8_t>, 4> movement_mouse_;
    std::array<Point, 4> movement_mouse_sizes_{ };
    Point mouse_position_{ -1, -1 }, hovered_cell_{ -1, -1 };
    Point hovered_{ -1, -1 };

public:
    // Raw inputs used by sub_476707; offsets refer to its runtime unit record.
    struct MovementContext
    {
        unsigned extra_flags{ }, status34{ }, status118{ }, status116{ };
        bool has_usable_ability{ };    // sub_443235(side, unit, 3) == 1
        unsigned movement_class{ }, sprite_height{ };
    };

private:
    std::vector<MovementContext> movement_contexts_;
    std::vector<OriginalEvents::Unit> movement_poses_;
    unsigned movement_elapsed_ = 0;
    unsigned movement_interval_ = 80;
    int moving_unit_ = -1;
    bool movement_animation_enabled_ = false;
    bool attack_animation_ = false;
    original_combat::Feedback combat_feedback_;
    bool curse33_feedback_{ };
    bool skill94_feedback_{ }, skill94_feedback_presented_{ }, skill94_death_lock_{ };
    std::function<void()> skill94_feedback_commit_;
    std::optional<OriginalEvents::Unit> reaction_pose_;
    std::function<void(std::vector<OriginalEvents::Unit>&, size_t, size_t)> death_settlement_;

public:
    struct PhysicalSettlement
    {
        size_t actor{ }, target{ };
        unsigned actor_id{ }, target_id{ };
        int hp_lost{ }, raw_xp{ };
        bool killed{ };
    };

private:
    std::function<void(std::vector<OriginalEvents::Unit>&, const PhysicalSettlement&)> physical_settlement_;
    std::function<Result<bool>(const std::vector<OriginalEvents::Unit>&, const std::vector<PhysicalSettlement>&)> physical_loot_commit_;
    std::function<Result<bool>(size_t, size_t)> physical_death_presenter_;
    std::function<void(bool)> physical_transaction_finish_;
    std::shared_ptr<OriginalEvents> hostile_resources_;
    std::shared_ptr<OriginalPromotionTransaction> physical_equipment_;
    std::vector<int> ai_kind_classes_;
    std::vector<std::array<std::vector<uint8_t>, 30>> action_sequences_;
    std::array<int, 0x110> ai_config_{ };
    size_t hostile_cursor_ = 0;
    int hostile_actor_ = -1, hostile_target_ = -1, hostile_attack_slot_ = -1;
    unsigned hostile_wait_ = 0;
    bool hostile_actions_active_ = false;
    bool controller_hostile_ = true;
    bool automatic_actions_completed_ = false;
    std::vector<Point> hostile_path_;
    size_t hostile_path_cursor_ = 0;
    bool hostile_retreat_ = false;
    bool hostile_special_ = false;

    bool original_rules_ = false;
    bool mask_surfaces_ = false;
    original_combat::Random random_{ 1 };
    std::optional<original_combat::Random> teleport_random_baseline_;
    std::optional<uint32_t> random_seconds_;
    int selected_ = -1;
    bool auto_selection_ = false;
    size_t next_player_unit_ = 0;

public:
    struct AiCandidateDiagnostic
    {
        unsigned actor{ }, target{ }, action{ };
        int chance{ }, damage{ }, value{ };
    };
    std::vector<AiCandidateDiagnostic> ai_candidate_diagnostics;
    struct PhysicalDiagnostic
    {
        size_t actor{ }, target{ };
        int legacy_chance{ }, original_chance{ }, accuracy_bonus{ }, height{ }, mode{ };
    };
    std::optional<PhysicalDiagnostic> last_physical_diagnostic_;
    const std::optional<PhysicalDiagnostic>& physical_diagnostic() const { return last_physical_diagnostic_; }

public:
    enum class State
    {
        idle,
        command,
        movement,
        attack_menu,
        attack_target,
        finish_confirm
    };
    static constexpr std::array<unsigned, 6> command_icons{ 158, 160, 162, 164, 188, 166 };
    // sub_43AE24 copies these six availability masks before packing the
    // surviving entries into the original 32-pixel command strip.
    static constexpr std::array<uint32_t, 6> command_masks{
        0x80000000u, 0x40000000u, 0x20000000u,
        0x10000000u, 0x00010000u, 0x08000000u
    };
    static constexpr std::array<unsigned, 8> attack_icons{ 168, 170, 172, 174, 176, 178, 180, 182 };
    static constexpr std::array<uint32_t, 8> attack_masks{
        0x04000000u, 0x02000000u, 0x01000000u, 0x00800000u,
        0x00400000u, 0x00200000u, 0x00100000u, 0x00080000u
    };

private:
    State state_ = State::idle;
    int command_ = 0, target_ = -1, attack_index_ = 0, requested_command_ = -1;
    std::function<bool(const OriginalEvents::Unit&, int)> command_request_available_;

public:
    int requested_command() const { return requested_command_; }
    void clear_requested_command() { requested_command_ = -1; }
    void commit_external_action() { acted_ = true; }
    // Modal owners supply their proven complete skill/promotion eligibility.
    void set_command_request_availability(std::function<bool(const OriginalEvents::Unit&, int)> available) { command_request_available_ = std::move(available); }

private:
    bool moved_ = false, acted_ = false;
    OriginalEvents::Unit before_move_{ };
    std::vector<int> distances_, previous_;
    std::vector<uint8_t> movement_categories_;
    std::vector<Point> path_;
    std::string message_ = "Select a unit to open its original command bar";
    static unsigned word(const auto& b, size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    uint32_t random_seconds() const { return random_seconds_.value_or(uint32_t(std::time(nullptr))); }
    int percent() { return random_.percent(random_seconds()); }
    int roll(int n) { return random_.bounded(n, random_seconds()); }

public:
    Battle(unsigned w, unsigned h, std::vector<Cell> cells) : width_(w), height_(h), cells_(std::move(cells))
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
    static Result<Battle> try_load(const std::filesystem::path& root, unsigned map, OriginalDisc disc = OriginalDisc::one)
    {
        auto prgResult = read_resource(root / "SUPERSP.PRG"), palResult = read_resource(root / "SUPERSP.PAL");
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
        auto binResult = read_resource(root / "SUPERSP.BIN"), kindsResult = read_resource(root / "KINDSP.BIN");
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
        }
        return Result<Battle>::success(std::move(result));
    }
    static Battle load(const std::filesystem::path& root, unsigned map, OriginalDisc disc = OriginalDisc::one)
    {
        auto result = try_load(root, map, disc);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    bool valid(Point p) const { return p.x >= 0 && p.y >= 0 && p.y < int(height_) && p.x < int(width_) - (p.y & 1); }
    Cell cell(Point p) const { return valid(p) ? cells_.at(size_t(p.y) * width_ + p.x) : Cell{ }; }
    void clear_runtime_object(Point p)
    {
        if (valid(p))
        {
            cells_.at(size_t(p.y) * width_ + p.x).object = 0;
        }
    }
    // 42ACF4 differs from storage bounds: border and raised top-edge cells are excluded.
    unsigned object_category(Point p) const
    {
        auto object = cell(p).object;
        return object > 0 && size_t(object) <= object_attributes_.size() ? object_attributes_[size_t(object - 1)] >> 8 : 0;
    }
    // 455828 allied control mode0 excludes ordinary NPC selector1.
    void settle_object_flags(OriginalEvents& events) const
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
    const std::vector<Point>& teleport_pads() const { return teleport_pads_; }
    void set_teleport_presenter(std::function<bool(unsigned, Point)> presenter) { teleport_presenter_ = std::move(presenter); }
    bool teleport_pending() const { return teleport_pending_; }
    void rollback_teleport()
    {
        if (teleport_random_baseline_)
        {
            random_ = *teleport_random_baseline_;
        }
        teleport_random_baseline_.reset();
        teleport_pending_ = false;
        teleport_destination_.reset();
        teleport_cursor_ = 0;
    }
    Result<bool> try_finish_teleport()
    {
        if (!teleport_pending_)
        {
            return Result<bool>::failure(make_error("No pending45DB10"));
        }
        teleport_pending_ = false;
        teleport_destination_.reset();
        teleport_random_baseline_.reset();
        ++teleport_cursor_;
        return Result<bool>::success(true);
    }
    void finish_teleport()
    {
        auto result = try_finish_teleport();
        if (!result)
        {
            report_error(result.error());
        }
    }
    // 4608B2 case35 -> 42FFA2 -> 45DB10, once per unit at side end.
    unsigned settle_teleport_pads(std::vector<OriginalEvents::Unit>& units, bool hostile)
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
    bool target_surface(Point p) const
    {
        if (p.x < 0 || p.y < 0 || (!(p.y & 1) && p.x < 1) || p.x >= int(width_) - 1 || p.y >= int(height_) - 1)
        {
            return false;
        }
        int elevation = cell(p).elevation;
        return p.y - (elevation > 0 ? elevation - 1 : 0) >= 1 && std::find(sealed_cells_.begin(), sealed_cells_.end(), p) == sealed_cells_.end();
    }
    struct TerrainEffect
    {
        int result{ };
        std::array<uint8_t, 3> bytes{ 1, 0, 0 };
    };
    TerrainEffect terrain_effect(const OriginalEvents& resources, const OriginalEvents::Unit& unit, Point p) const
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
    bool target_visible(const OriginalEvents::Unit& target, std::span<const OriginalEvents::Unit> opposing, unsigned opposing_mode) const
    {
        if (!(target.abilities & 0x80) && !(target.status3c & 0x10))
        {
            return true;
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
    std::vector<Point> action_cells(Point origin, OriginalEvents::ActionRange range) const
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
    // 408455 compares the attacker's row parity, not the defender's.
    static unsigned attack_mode(Point attacker, Point defender, unsigned defender_direction, bool simple_rules = false)
    {
        if (simple_rules)
        {
            return 1;
        }
        bool left = (attacker.y & 1) ? attacker.x < defender.x : attacker.x <= defender.x;
        unsigned rear = left ? (attacker.y > defender.y ? 2u : 1u) : (attacker.y > defender.y ? 3u : 0u);
        unsigned front = left ? (attacker.y > defender.y ? 0u : 3u) : (attacker.y > defender.y ? 1u : 2u);
        return defender_direction == rear ? 3 : defender_direction == front ? 1 :
                                                                              2;
    }
    struct ActionTarget
    {
        size_t unit_index{ };
        Point position;
        unsigned direction{ };
    };
    std::vector<ActionTarget> action_targets(const OriginalEvents& resources, const OriginalEvents::Unit& actor,
        const OriginalEvents::ActionRecord& action, std::span<const OriginalEvents::Unit> candidates,
        std::span<const OriginalEvents::Unit> opposing, unsigned opposing_mode) const
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
    std::vector<unsigned> counter_actions(const OriginalEvents& resources, const OriginalEvents::Unit& defender,
        Point attacker_position, std::span<const OriginalEvents::Unit> attackers, std::span<const OriginalEvents::Unit> defenders,
        unsigned attacker_mode, uint8_t defender_39, uint32_t defender_status3c_dword) const
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
    Result<std::vector<size_t>> try_physical_target_group(const OriginalEvents& resources, const OriginalEvents::Unit& actor, const OriginalEvents::ActionRecord& action, const std::vector<OriginalEvents::Unit>& units, size_t primary) const
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
    struct PhysicalActionContext
    {
        size_t actor{ }, target{ };
        OriginalEvents::Unit actor_snapshot, target_snapshot;
        OriginalEvents::ActionRecord action{ };
        std::optional<OriginalEvents::ActionRecord> counter;
        int mode{ }, counter_tactics{ };
    };
    static uint8_t physical_status(const OriginalEvents::Unit& unit, unsigned offset)
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
    static uint32_t physical_status_dword(const OriginalEvents::Unit& unit)
    {
        uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i) { value |= uint32_t(physical_status(unit, 0x3C + i)) << (8 * i); }
        return value;
    }
    Result<std::vector<PhysicalActionContext>> try_physical_contexts(const OriginalEvents& resources, const std::vector<OriginalEvents::Unit>& units, size_t actor, size_t primary, const OriginalEvents::ActionRecord& action, bool reuse_primary = false)
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
            PhysicalActionContext context{ };
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
    struct AttackAvailability
    {
        uint32_t mask{ };
        std::array<int, 8> actions{ -1, -1, -1, -1, -1, -1, -1, -1 };
    };
    AttackAvailability attack_availability(const OriginalEvents& resources, const OriginalEvents::Unit& actor,
        std::span<const OriginalEvents::Unit> candidates, std::span<const OriginalEvents::Unit> opposing, unsigned opposing_mode) const
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
            unsigned slot = type == 5 ? 1 : type >= 1 && type <= 7 ? 0 :
                type == 23                                         ? 6 :
                type == 32                                         ? 5 :
                type == 36                                         ? 4 :
                                                                     7;
            result.mask |= attack_masks[slot];
            result.actions[slot] = int(action.index);
        }
        if (result.mask)
        {
            result.mask |= command_masks[0];
        }
        return result;
    }
    static int original_distance(Point from, Point to)
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
    // sub_448CD6: four diagonal neighbours in the staggered grid.
    static bool adjacent(Point a, Point b) { return std::abs(a.y - b.y) == 1 && (b.x == a.x || b.x == a.x + ((a.y & 1) ? 1 : -1)); }
    static Point center(Point p) { return { p.x * 64 + (p.y & 1) * 32 + 32, p.y * 16 + 16 }; }
    Point pick(float x, float y, int camera_x, int camera_y) const
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
    bool orient_finish_threat(const OriginalEvents& resources, std::vector<OriginalEvents::Unit>& units)
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
        std::vector<original_ai::MovementTarget> targets;
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
        const auto* target = original_ai::original_target(targets, { actor.x, actor.y }, 0);
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
    void finish_action(std::vector<OriginalEvents::Unit>& units, bool confirmed)
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
        if (original_rules_ && unit.action_points >= config_[8])
        {
            unit.action_points = int(int16_t(unit.action_points - config_[8]));
            unit.carry_points = std::min(unit.carry_points, unit.action_points);
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
    Result<bool> try_restore_turn(unsigned value)
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
    void restore_turn(unsigned value)
    {
        auto result = try_restore_turn(value);
        if (!result)
        {
            report_error(result.error());
        }
    }
    int selected() const { return selected_; }
    // sub_4372EF reads 4F7D04's high byte (SET field four), not field six.
    void set_auto_selection(bool enabled) { auto_selection_ = enabled; }
    bool player_selection_eligible(const OriginalEvents::Unit& unit, unsigned context_flags = 0) const
    {
        if (!unit.present(turn_) || !unit.controllable() || !(unit.status_flags & 0x80) || (unit.status_flags & 0x20) || unit.action_state == 2)
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
    bool select_unit(std::vector<OriginalEvents::Unit>& units, size_t index)
    {
        if (hostile_side_ || hostile_actions_active_ || animating_movement() || combat_feedback_active() || state_ != State::idle || index >= units.size())
        {
            return false;
        }
        if (!units[index].present(turn_) || !units[index].controllable() || units[index].action_state == 2 || !valid({ units[index].x, units[index].y }))
        {
            return false;
        }
        selected_ = int(index);
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
    bool select_next_player_unit(std::vector<OriginalEvents::Unit>& units, bool restart = false, unsigned context_flags = 0)
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
            if (!select_unit(units, index))
            {
                return false;
            }
            next_player_unit_ = (index + 1) % units.size();
            return true;
        }
        return false;
    }
    const std::string& message() const { return message_; }
    State state() const { return state_; }
    int command_index() const { return command_; }
    int target() const { return target_; }
    // This is the six-bit portion of dword_50049C consumed by sub_43AE24.
    // It deliberately exposes availability rather than inventing labels: the
    // renderer uses the same source icon IDs and marks unavailable slots.
    uint32_t command_availability(const std::vector<OriginalEvents::Unit>& units) const
    {
        if (selected_ < 0 || size_t(selected_) >= units.size())
        {
            return 0;
        }
        const auto& u = units.at(selected_);
        if (!u.present(turn_) || !u.controllable() || (original_rules_ && (!(u.status_flags & 0x80) || (u.status_flags & 0x20))))
        {
            return 0;
        }
        uint32_t mask = command_masks[5] | (original_rules_ ? command_masks[3] : 0);
        if ((!acted_ || original_rules_) && u.action_points >= (original_rules_ ? attack_menu_cost(u) : attack_cost(u)))
        {
            mask |= command_masks[0];
        }
        const auto context = size_t(selected_) < movement_contexts_.size() ? movement_contexts_[selected_] : MovementContext{ };
        bool move_budget = !original_rules_ || u.max_action_points >= config_[0] + config_[context.movement_class ? 1 : 4];
        if (!moved_ && !(u.status34 & 1) && move_budget)
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
    void refresh_command_availability(const std::vector<OriginalEvents::Unit>& units) { available_mask_ = command_availability(units); }
    bool command_available(const std::vector<OriginalEvents::Unit>& units, int index) const
    {
        return index >= 0 && index < 6 && (command_availability(units) & command_masks[size_t(index)]) != 0;
    }
    int command_slot(const std::vector<OriginalEvents::Unit>& units, int command) const
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
    int command_from_slot(const std::vector<OriginalEvents::Unit>& units, int slot) const
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
    int command_count(const std::vector<OriginalEvents::Unit>& units) const
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
    void prepare_commands(const std::vector<OriginalEvents::Unit>& units)
    {
        available_mask_ = command_availability(units);
        if (command_ < 0 || command_ >= 6 || !command_available_for_cycle(command_))
        {
            command_ = command_from_slot(units, 0);
        }
    }
    const std::vector<int>& route_costs() const { return distances_; }
    const std::vector<Point>& path() const { return path_; }
    // sub_477BEB compares signed remaining AP, not path length or terrain.
    static uint8_t movement_category(int remaining, unsigned flags, int ability_threshold, int attack_threshold)
    {
        if ((flags & 2) && remaining >= ability_threshold)
        {
            return 2;
        }
        if ((flags & 1) && remaining >= attack_threshold)
        {
            return 1;
        }
        return 0;
    }
    void set_movement_contexts(std::vector<MovementContext> contexts) { movement_contexts_ = std::move(contexts); }
    const std::vector<uint8_t>& movement_categories() const { return movement_categories_; }
    // Optional raw destination-record override, applied after build_routes.
    // Normal operation generates these values in build_routes itself.
    void set_movement_categories(const std::vector<uint8_t>& categories)
    {
        movement_categories_.assign(cells_.size(), 0);
        for (size_t i = 0; i < movement_categories_.size() && i < categories.size(); ++i)
        {
            movement_categories_[i] = categories[i] <= 2 ? categories[i] : 0;
        }
    }
    const auto& physical_exchange_diagnostics() const { return physical_exchange_diagnostics_; }
    const original_combat::Feedback& combat_feedback() const { return combat_feedback_; }
    bool combat_feedback_active() const { return combat_feedback_.active() || skill94_death_lock_ || physical_exchange_.has_value(); }
    void lock_skill94_death(bool locked) { skill94_death_lock_ = locked; }
    void feedback_presented()
    {
        if (skill94_feedback_)
        {
            skill94_feedback_presented_ = true;
        }
    }
    bool begin_skill94_feedback(size_t actor, size_t target, int hp, int damage, std::function<void()> commit)
    {
        if (combat_feedback_active() || !commit || damage < 0 || damage > hp)
        {
            return false;
        }
        combat_feedback_.begin(actor, target, hp, damage, damage == 0, false);
        // 468BAC starts directly at number step0: no physical452E7F flash.
        combat_feedback_.phase = original_combat::Feedback::Phase::numbers;
        combat_feedback_.consume();
        skill94_feedback_ = true;
        skill94_feedback_presented_ = false;
        skill94_feedback_commit_ = std::move(commit);
        return true;
    }
    bool begin_curse33_feedback(size_t actor, size_t target, int hp, bool hit, std::function<void()> commit)
    {
        if (!begin_skill94_feedback(actor, target, hp, 0, std::move(commit))) { return false; }
        // 4677D7: miss goes to468BAC(0); success uses452E7F without a number.
        curse33_feedback_ = hit;
        if (hit) { combat_feedback_.phase = original_combat::Feedback::Phase::flash; }
        return true;
    }
    template <class ScreenPosition, class Silhouette, class Text>
    void render_combat_feedback(const std::vector<OriginalEvents::Unit>& units, ScreenPosition screen_position, Silhouette silhouette, Text text) const
    {
        const auto& f = combat_feedback_;
        if (!f.active() || f.target >= units.size())
        {
            return;
        }
        auto unit = units[f.target];
        if (skill94_feedback_)
        {
            unit.hp = f.hp_after;
        }
        auto position = screen_position(unit);
        if (f.silhouette())
        {
            silhouette(unit, position, f.silhouette_palette());
        }
        if (f.text_visible())
        {
            text(f.text(), position.x + f.text_x(), position.y + f.text_y(), f.text_foreground(), f.text_background());
        }
    }
    void set_death_settlement(std::function<void(std::vector<OriginalEvents::Unit>&, size_t, size_t)> callback) { death_settlement_ = std::move(callback); }
    void set_physical_settlement(std::function<void(std::vector<OriginalEvents::Unit>&, const PhysicalSettlement&)> callback) { physical_settlement_ = std::move(callback); }
    void set_physical_loot_commit(std::function<Result<bool>(const std::vector<OriginalEvents::Unit>&, const std::vector<PhysicalSettlement>&)> callback) { physical_loot_commit_ = std::move(callback); }
    void set_physical_death_presenter(std::function<Result<bool>(size_t, size_t)> callback, std::function<void(bool)> finish)
    {
        physical_death_presenter_ = std::move(callback);
        physical_transaction_finish_ = std::move(finish);
    }
    void resume_physical_death()
    {
        if (physical_exchange_)
        {
            physical_exchange_->death.reset();
            physical_exchange_->death_started = false;
        }
    }

private:
    struct PhysicalExchangeDiagnostic
    {
        size_t actor{ }, target{ };
        int damage{ };
        bool reverse{ }, consecutive{ };
    };
    std::vector<PhysicalExchangeDiagnostic> physical_exchange_diagnostics_;
    struct PhysicalExchangeStep
    {
        PhysicalActionContext context;
        int damage{ };
        bool consecutive{ }, reverse{ };
    };
    struct PhysicalExchange
    {
        std::vector<PhysicalExchangeStep> forward, replies;
        std::vector<PhysicalSettlement> forward_awards, counter_awards;
        std::vector<OriginalEvents::Unit> forward_targets, baseline_units;
        original_combat::Random baseline_random{ 1 };
        std::vector<std::pair<size_t, size_t>> deaths;
        size_t cursor{ }, counter_cursor{ };
        std::optional<std::pair<size_t, size_t>> death;
        bool death_started{ };
        bool reverse{ }, group_presented{ }, group_status_applied{ };
    };
    std::optional<PhysicalExchange> physical_exchange_;
    std::optional<OriginalEvents::ActionRecord> pending_action_status_;
    int apply_physical_action_status(std::vector<OriginalEvents::Unit>& units, size_t b, const OriginalEvents::ActionRecord& action, int result, std::optional<int> status_roll = std::nullopt)
    {
        auto& target = units[b];
        original_combat::ActionStatusInput input{ };
        input.result = result;
        input.roll = status_roll ? *status_roll : (result > 0 ? percent() : 100);
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
    int apply_physical_status_tail(std::vector<OriginalEvents::Unit>& units, size_t actor, size_t target, const OriginalEvents::ActionRecord& action, int damage)
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
                    original_combat::apply_timed_status(*victim.action_status_runtime, 0x4000, 3);
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
    void settle_physical(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int lost)
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

public:
    void tick_combat_feedback(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds)
    {
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
                feedback.phase = original_combat::Feedback::Phase::idle;
                curse33_feedback_ = skill94_feedback_ = skill94_feedback_presented_ = false;
                auto commit = std::move(skill94_feedback_commit_);
                commit();
                return;
            }
            unsigned interval = feedback.step < 3 ? 30u : feedback.step < 4 ? 20u :
                                                                              10u;
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
                feedback.phase = original_combat::Feedback::Phase::idle;
                skill94_feedback_ = false;
                auto commit = std::move(skill94_feedback_commit_);
                commit();
                return;
            }
            feedback.consume();
            return;
        }
        if (feedback.phase == original_combat::Feedback::Phase::windup)
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
            feedback.phase = original_combat::Feedback::Phase::flash;
            feedback.elapsed = 0;
            return;
        }
        feedback.elapsed += std::min(milliseconds, 1000u);
        if (feedback.phase == original_combat::Feedback::Phase::flash)
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
            feedback.phase = original_combat::Feedback::Phase::numbers;
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
            feedback.phase = original_combat::Feedback::Phase::idle;
            settle_physical(units, a, b, feedback.damage);
            return;
        }
        units.at(feedback.target).hp -= feedback.consume();
    }
    bool movement_presentation_driven_{ }, movement_presented_{ };
    void enable_movement_presentation() { movement_presentation_driven_ = true; }
    void movement_presented()
    {
        if (animating_movement()) { movement_presented_ = true; }
    }
    void enable_movement_animation() { movement_animation_enabled_ = true; }
    bool animating_movement() const { return moving_unit_ >= 0; }
    bool attack_animation_active() const { return attack_animation_ && animating_movement(); }
    void tick_movement(unsigned milliseconds)
    {
        if (!animating_movement())
        {
            return;
        }
        // 452167 resets its clock after each presented pose. Discard frame
        // overruns instead of carrying them into (or skipping) the next pose.
        unsigned frame = movement_elapsed_ / movement_interval_;
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
                moving_unit_ = -1;
                movement_poses_.clear();
                movement_elapsed_ = 0;
                attack_animation_ = false;
                return;
            }
            movement_elapsed_ = frame * movement_interval_;
        }
        else
        {
            movement_elapsed_ = frame * movement_interval_ + elapsed;
        }
    }
    std::vector<OriginalEvents::Unit> render_units(const std::vector<OriginalEvents::Unit>& units) const
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
        return result;
    }
    void cancel()
    {
        if (animating_movement() || combat_feedback_active())
        {
            return;
        }
        path_.clear();
        target_ = -1;
        if (state_ == State::movement || state_ == State::attack_menu || state_ == State::attack_target)
        {
            state_ = State::command;
            message_ = "Command cancelled; select another command";
            return;
        }
        selected_ = -1;
        state_ = State::idle;
        message_ = "Select a unit to open its original command bar";
    }
    void cancel(std::vector<OriginalEvents::Unit>& units)
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
    static Point surface_center(Point p, Cell c)
    {
        auto q = center(p);
        q.y -= std::max(0, c.elevation - 1) * 16;
        return q;
    }
    Point screen_center(Point p, int cx, int cy) const
    {
        auto q = surface_center(p, cell(p));
        return { q.x - cx, q.y - cy };
    }
    void set_mask_surfaces(bool enabled) { mask_surfaces_ = enabled; }
    Point pick_surface(float x, float y, int cx, int cy) const
    {
        if (original_rules_)
        {
            // 42A2BD/42A3D2: integer division truncates toward zero, including negative edges.
            int half = cx / 32, camera_row = cy / 16;
            int mx = int(x) + ((half & 1) ? 32 : 0), my = int(y) + ((camera_row & 1) ? 16 : 0);
            int left = -((mx + 2 * my - 32) / 64) * 64 - 32;
            int right = ((2 * my - mx + 608) / 64) * 64 - 608;
            int anchor_x = (-left - right) / 2, anchor_y = (right - left) / 4;
            int local_x = mx - anchor_x + 32, local_y = my - anchor_y;
            int quadrant = 2 * local_y + local_x < 64 ? (local_x - 2 * local_y < 0 ? 3 : 1) : (local_x - 2 * local_y < 0 ? 2 : 4);
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
    Point command_origin(const std::vector<OriginalEvents::Unit>& units, int cx, int cy) const
    {
        if (selected_ < 0)
        {
            return { -1, -1 };
        }
        auto c = screen_center({ units[selected_].x, units[selected_].y }, cx, cy);
        int count = state_ == State::attack_menu ? std::max(1, attack_count(units)) : std::max(1, command_count(units));
        const auto context = size_t(selected_) < movement_contexts_.size() ? movement_contexts_[selected_] : MovementContext{ };
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
    // 477DE4 rejects hostile occupants; 477E50 only clears the landing marker
    // for allied occupants. Friendly roles block destinations, not transit.
    bool player_route_passable(const std::vector<OriginalEvents::Unit>& units, const OriginalEvents::Unit& actor, Point p) const
    {
        int i = occupant(units, p);
        return i < 0 || (original_rules_ && units[size_t(i)].hostile() == actor.hostile());
    }
    void build_routes(const std::vector<OriginalEvents::Unit>& units)
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
            const auto context = size_t(selected_) < movement_contexts_.size() ? movement_contexts_[selected_] : MovementContext{ };
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
    std::vector<Point> occupied_route_destinations_;
    bool preview(Point p)
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
    int attack_menu_cost(const OriginalEvents::Unit& unit) const
    {
        // 43B096 reads dwords at EB/F0 then SAR 24: the signed bytes are EE/F3.
        return attack_cost(unit);
    }
    int attack_cost(const OriginalEvents::Unit& unit) const
    {
        int cost = config_[6] - ((unit.abilities & 0x20000000u) ? 4 : 0);
        if ((unit.status34 & 2) && !(unit.abilities & 0x8000))
        {
            cost += config_[11];
        }
        return cost;
    }
    static int executable_attack_slot(const OriginalEvents::ActionRecord& action)
    {
        int type = action.fields[0];
        static constexpr std::array<int, 23> supported{ 40, -84, 114, -92, 98, 0, 0, 0, 0, 0, 3, 9, 90, 0, 0, 116, 0, 1, 65, 4, 0, 0, 1 };
        static constexpr std::array<int, 23> alternate{ 40, -74, 87, -76, -25, 0, 0, 0, 0, 0, 4, 8, 70, 0, 2, -96, 0, 9, 50, 0, 0, 125, 1 };
        static constexpr std::array<int, 23> ranged{ 40, -91, -49, -92, -30, -67, 98, 0, 0, 0, 3, 6, 70, 2, 6, 119, 0, 0, 0, 0, 2, 0, 2 };
        return type == 5 ? 1 : type >= 1 && type <= 7                                                                                                                      ? 0 :
            (action.index == 163 && action.fields == supported) || (action.index == 130 && action.fields == alternate) || (action.index == 160 && action.fields == ranged) ? 7 :
                                                                                                                                                                             -1;
    }
    std::optional<OriginalEvents::ActionRecord> executable_action(const std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot = -1) const
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
        std::array<bool, 8> reaches{ };
        for (const auto& action : hostile_resources_->action_records(u))
        {
            int action_slot = executable_attack_slot(action);
            if (action_slot < 0 || (slot >= 0 && slot != action_slot))
            {
                continue;
            }
            // Preserve the existing bounded contact executor. Type5 uses its
            // decoded original range, including minimum and shape/height gates.
            if (action_slot == 0 && (!adjacent({ u.x, u.y }, { v.x, v.y }) || cell({ u.x, u.y }).elevation != cell({ v.x, v.y }).elevation))
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
    bool can_attack(const std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot = -1) const
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
    bool attack_command_available(const std::vector<OriginalEvents::Unit>& units, int index) const
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
    int attack_index() const { return attack_index_; }
    int attack_count(const std::vector<OriginalEvents::Unit>& units) const
    {
        int count = 0;
        for (int i = 0; i < 8; ++i)
        {
            count += attack_command_available(units, i);
        }
        return count;
    }
    int attack_from_slot(const std::vector<OriginalEvents::Unit>& units, int slot) const
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
    void cycle_command(int delta)
    {
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
    bool command_available_for_cycle(int index) const
    {
        return (command_masks[size_t(index)] & available_mask_) != 0;
    }

private:
    uint32_t available_mask_{ };
    unsigned attack_menu_mask_{ };

public:
    void cycle_target(const std::vector<OriginalEvents::Unit>& units)
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
    bool activate(std::vector<OriginalEvents::Unit>& units)
    {
        if (animating_movement() || combat_feedback_active())
        {
            return false;
        }
        if (selected_ < 0)
        {
            return false;
        }
        if (state_ == State::attack_menu)
        {
            if (!attack_command_available(units, attack_index_))
            {
                return false;
            }
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
        if (command_ == 2 || command_ == 3 || command_ == 4)
        {
            requested_command_ = command_;
            return true;
        }
        message_ = "Original command not implemented; no action performed";
        return false;
    }
    void hover(const std::vector<OriginalEvents::Unit>& units, Point p)
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
    int attack_target_at(const std::vector<OriginalEvents::Unit>& units, Point p) const
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
    bool mouse(std::vector<OriginalEvents::Unit>& units, float x, float y, bool pressed, int cx, int cy)
    {
        mouse_position_ = { int(x), int(y) };
        if (animating_movement() || combat_feedback_active())
        {
            return false;
        }
        if (state_ == State::command || state_ == State::attack_menu)
        {
            prepare_commands(units);
            auto o = command_origin(units, cx, cy);
            int count = state_ == State::attack_menu ? attack_count(units) : command_count(units);
            if (x >= o.x && x < o.x + count * 32 && y >= o.y && y < o.y + 32)
            {
                if (state_ == State::command)
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
        return pressed ? click(units, p) : false;
    }
    int occupant(const std::vector<OriginalEvents::Unit>& units, Point p) const
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
    int step_cost(const OriginalEvents::Unit& u, Point from, Point to) const
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
    bool can_step(const std::vector<OriginalEvents::Unit>& units, size_t index, Point to) const
    {
        if (index >= units.size() || !units[index].present(turn_) || !units[index].controllable() || (original_rules_ && (!(units[index].status_flags & 0x80) || (units[index].status_flags & 0x20))))
        {
            return false;
        }
        const auto& u = units[index];
        int cost = step_cost(u, { u.x, u.y }, to);
        return cost >= 0 && occupant(units, to) < 0 && (!original_rules_ || cost <= std::min(110, u.action_points - config_[0]));
    }
    std::optional<std::vector<Point>> scripted_route(const OriginalEvents::Unit& unit, Point destination) const
    {
        using namespace original_ai;
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
                    int distance = original_ai::original_distance({ destination.x, destination.y }, p), s = (destination.x - p.x) * (destination.x - p.x) + (destination.y - p.y) * (destination.y - p.y);
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
    unsigned scripted_frame_count(const OriginalEvents::Unit& unit, unsigned facing) const
    {
        if (!unit.appearance || unit.appearance > action_sequences_.size())
        {
            return 0;
        }
        return unsigned(action_sequences_[unit.appearance - 1][4 + (facing & 3)].size());
    }
    int hovered_unit(const std::vector<OriginalEvents::Unit>& units) const { return valid(hovered_) ? occupant(units, hovered_) : -1; }
    static unsigned facing(Point from, Point to, unsigned current)
    {
        if (from.x == to.x && from.y == to.y)
        {
            return current;
        }
        bool right = (from.y & 1) ? from.x < to.x : from.x <= to.x;
        return right ? (from.y > to.y ? 2u : 1u) : (from.y > to.y ? 3u : 0u);
    }
    bool step(std::vector<OriginalEvents::Unit>& units, size_t index, Point to, bool allied_transit = false)
    {
        if (!can_step(units, index, to))
        {
            if (!allied_transit || index >= units.size() || !units[index].controllable() || !units[index].present(turn_) || !(units[index].status_flags & 0x80) || (units[index].status_flags & 0x20) || !player_route_passable(units, units[index], to))
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
    // sub_408CB7 / sub_40A622: equal-height base arithmetic only; class/status/skill modifiers remain pending.
    static int ordinary_damage(int attack, int defense, int divisor, int minimum_roll)
    {
        return original_combat::ordinary_damage(attack, defense, divisor, minimum_roll);
    }
    void seed(unsigned value)
    {
        random_seconds_ = value;
        random_ = original_combat::Random(1);
        random_.initialize(value);
    }
    // Callback must stage its own domain mutations; failure leaves the
    // combat stream untouched. Permutation initialization is never repeated.
    bool mutate_combat_random(const std::function<bool(original_combat::Random&, uint32_t)>& transaction)
    {
        auto staged = random_;
        if (!transaction(staged, random_seconds()))
        {
            return false;
        }
        random_ = staged;
        return true;
    }
    bool present_skill32_preparation(std::vector<OriginalEvents::Unit>& units, size_t index, size_t target, bool second = false)
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
            facing = right ? (actor.y > destination.y ? 2u : 1u) : (actor.y > destination.y ? 3u : 0u);
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
        return true;
    }
    bool mutate_random(const std::function<bool(OriginalRandom&, uint32_t)>& transaction)
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
    std::array<uint8_t, 3> original_42c88f_bytes(const OriginalEvents& resources, const OriginalEvents::Unit& unit) const { return terrain_effect(resources, unit, { unit.x, unit.y }).bytes; }
    unsigned turn() const { return turn_; }
    bool hostile_side() const { return hostile_side_; }
    bool hostile_actions_active() const { return hostile_actions_active_; }
    int moving_unit() const { return moving_unit_; }
    int moving_actor() const { return moving_unit_; }
    void set_hostile_resources(const OriginalEvents& resources) { hostile_resources_ = std::make_shared<OriginalEvents>(resources); }
    int physical_action_preview(const OriginalEvents& resources, const OriginalEvents::Unit& actor, const OriginalEvents::Unit& defender, const OriginalEvents::ActionRecord& action, bool counter = false, int counter_mode = 0) const
    {
        original_combat::PhysicalPreviewInput in{ };
        in.attack = actor.attack;
        in.defense = defender.defense;
        in.context_bonus = int(resources.action_proficiency(actor, action));
        in.height_difference = original_combat::terrain_height_difference(cell({ actor.x, actor.y }).elevation, cell({ defender.x, defender.y }).elevation, resources.difficulty == 1);
        in.player_side = !actor.hostile();
        in.difficulty_bonus = ai_config_[0xF3 + std::min(5u, resources.difficulty)];
        int mode = int(attack_mode({ actor.x, actor.y }, { defender.x, defender.y }, defender.direction, resources.difficulty == 1));
        in.counter = counter;
        in.counter_fixed_damage = counter && (actor.abilities & 0x400);
        in.counter_fixed_value = actor.strikeback;
        in.counter_equipment_30 = counter && resources.equipment_special(actor, 30);
        in.mode_bonus = counter ? (counter_mode == 3 ? -3 : counter_mode == 2 ? -2 :
                                                                                0) :
                                  (mode == 3 ? 6 : mode == 2 ? 3 :
                                                               0);
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
        in.attacker_3c = actor.status3c;
        in.attacker_118 = uint8_t(actor.abilities >> 24);
        in.defender_kind = uint8_t(defender.kind_attribute);
        uint8_t status3d = defender.action_status_runtime ? (*defender.action_status_runtime)[0x3D] : defender.status3d;
        in.immune = (defender.abilities & 0x01000000u) || (status3d & 8);
        // 409BBA+4C1 -> 407C35 uses damage bytes 0/1, not hit bytes 2/3.
        in.multiplier = original_combat::preview_multiplier(ai_config_[0], ai_config_[1]);
        return original_combat::physical_preview(in);
    }
    struct AiPlanDiagnostic
    {
        unsigned actor_id{ };
        int behavior{ }, appearance_kind{ }, dispatch{ };
        bool unsupported_goal{ }, unsupported_special{ }, no_attack_candidate{ };
        int movement_mode{ }, route_count{ }, destination_record{ };
        bool unsupported_health{ };
        unsigned target_id{ };
        int target_value{ };
        bool target_hidden{ };
    };
    std::optional<AiPlanDiagnostic> ai_plan_diagnostic_;
    const std::optional<AiPlanDiagnostic>& ai_plan_diagnostic() const { return ai_plan_diagnostic_; }
    bool npc_controller_ = false;
    bool automatic_actions_completed() const { return automatic_actions_completed_; }
    void begin_npc_actions(std::vector<OriginalEvents::Unit>&)
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
    bool tick_npc_actions(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds) { return tick_hostile_actions(units, milliseconds); }
    void begin_automatic_actions(std::vector<OriginalEvents::Unit>& units)
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
    bool tick_automatic_actions(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds)
    {
        if (controller_hostile_)
        {
            return automatic_actions_completed_;
        }
        return tick_hostile_actions(units, milliseconds);
    }
    void begin_hostile_actions(std::vector<OriginalEvents::Unit>& units)
    {
        if (!hostile_side_)
        {
            begin_hostile_side(units);
        }
        begin_controller_actions(true);
    }

private:
    void begin_controller_actions(bool hostile)
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
    static void idle_pose(OriginalEvents::Unit& unit)
    {
        unit.animation_action = 0;
        unit.movement_frame = 0;
        unit.movement_pose = false;
    }

public:
    // Incremental executor: one route edge is committed per visible pose interval.
    // Returns true only after the last actor has finished and side AP is reset.
    bool tick_hostile_actions(std::vector<OriginalEvents::Unit>& units, unsigned milliseconds)
    {
        if (!hostile_actions_active_)
        {
            return true;
        }
        if (animating_movement())
        {
            tick_movement(milliseconds);
            return false;
        }
        if (combat_feedback_active())
        {
            tick_combat_feedback(units, milliseconds);
            return false;
        }
        if (hostile_wait_)
        {
            hostile_wait_ -= std::min(hostile_wait_, milliseconds);
            return false;
        }
        if (hostile_actor_ >= 0)
        {
            auto& actor = units.at(hostile_actor_);
            if (actor.present(turn_) && hostile_path_cursor_ < hostile_path_.size())
            {
                Point next = hostile_path_[hostile_path_cursor_++];
                if (!advance_hostile(units, size_t(hostile_actor_), next))
                {
                    hostile_path_cursor_ = hostile_path_.size();
                }
                return false;
            }
            if (actor.present(turn_) && hostile_retreat_)
            {
                if (actor.retreat_role14 != 5 && !(actor.action_status_runtime && ((*actor.action_status_runtime)[0x38] & 2)))
                {
                    int kind = actor.appearance && actor.appearance <= ai_kind_classes_.size() ? ai_kind_classes_[actor.appearance - 1] : 0;
                    int value = hostile_resources_->combat_value(actor);
                    if (original_ai::retreat_40CFE3(0, kind, int16_t(actor.hp), int16_t(actor.max_hp), value, { hostile_resources_->combat_value(actor, 5), hostile_resources_->combat_value(actor, 6), hostile_resources_->combat_value(actor, 7), hostile_resources_->combat_value(actor, 8) }))
                    {
                        actor.retreat_role14 = 5;
                    }
                }
            }
            if (actor.present(turn_) && !hostile_retreat_ && !hostile_special_)
            {
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
                    return false;
                }
            }
            if (original_rules_ && actor.action_points >= config_[8])
            {
                actor.action_points = int(int16_t(actor.action_points - config_[8]));
                actor.carry_points = std::min(actor.carry_points, actor.action_points);
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
            bool eligible = controller_hostile_ ? units[index].hostile() : npc_controller_ ? !units[index].hostile() && units[index].behavior == 1 :
                                                                                             units[index].controllable();
            if (!eligible || !units[index].present(turn_) || units[index].action_state != 0)
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
    bool tick_hostile(const OriginalEvents& resources, std::vector<OriginalEvents::Unit>& units, unsigned milliseconds)
    {
        if (!hostile_actions_active_)
        {
            set_hostile_resources(resources);
            begin_hostile_actions(units);
        }
        return tick_hostile_actions(units, milliseconds);
    }

private:
    int hostile_contact_target(const std::vector<OriginalEvents::Unit>& units, size_t actor) const
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
    bool advance_hostile(std::vector<OriginalEvents::Unit>& units, size_t index, Point to)
    {
        auto& actor = units[index];
        int cost = step_cost(actor, { actor.x, actor.y }, to);
        if (cost < 0 || occupant(units, to) >= 0 || (original_rules_ && cost > actor.action_points - config_[0]))
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
        actor.movement_frame = saved.direction == actor.direction ? saved.movement_frame + (movement_class == 4 || movement_class == 5 ? 0 : 1) : 0;
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
    void plan_hostile(const std::vector<OriginalEvents::Unit>& units, size_t index)
    {
        if (!hostile_resources_)
        {
            return;
        }
        ai_candidate_diagnostics.clear();
        const auto& actor = units[index];
        using namespace original_ai;
        if (actor.action_points < attack_cost(actor))
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
                if (cost < 0 || occupied_unit(to))
                {
                    return std::nullopt;
                }
                return cost;
            },
            [&](Position p)
            {
                return occupied_unit(p) ? 0 : 1;
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
                        int distance = original_ai::original_distance(origin, p);
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
        Selection attack_selection;
        auto scan = [&](Position position, int record)
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
            for (const auto& action : hostile_resources_->action_records(actor))
            {
                if (executable_attack_slot(action) < 0)
                {
                    continue;
                }
                std::vector<Candidate> candidates;
                for (const auto& target : action_targets(*hostile_resources_, moved_actor, action, opponents, moved_observers, 0))
                {
                    const auto& defender = opponents[target.unit_index];
                    if (executable_attack_slot(action) == 0 && (!adjacent({ position.x, position.y }, { defender.x, defender.y }) || cell({ position.x, position.y }).elevation != cell({ defender.x, defender.y }).elevation))
                    {
                        continue;
                    }
                    int damage = physical_action_preview(*hostile_resources_, moved_actor, defender, action);
                    original_combat::HitChanceInput hit{ };
                    hit.preview = true;
                    hit.attacker_tactics = actor.tactics;
                    hit.defender_evasion = defender.strikeback;
                    hit.height_difference = original_combat::terrain_height_difference(cell({ position.x, position.y }).elevation, cell({ defender.x, defender.y }).elevation, hostile_resources_->difficulty == 1);
                    hit.attack_mode = int(attack_mode({ position.x, position.y }, { defender.x, defender.y }, defender.direction, hostile_resources_->difficulty == 1));
                    hit.context_bonus = int(hostile_resources_->action_proficiency(actor, action));
                    hit.random_multiplier = original_combat::preview_multiplier(ai_config_[2], ai_config_[3]);
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
                    int chance = original_combat::hit_chance(hit);
                    MeleePreview preview{ };
                    preview.attack_value = physical_preview_value(chance, damage);
                    preview.attack_damage = damage;
                    preview.attack_chance = chance;
                    preview.target_hp = defender.hp;
                    preview.attacker_hp = actor.hp;
                    preview.target_kind = int(defender.id);
                    ai_candidate_diagnostics.push_back({ actor.id, defender.id, action.index, chance, damage, preview.attack_value });
                    candidates.push_back({ tokens[target.unit_index], record, preview, int(action.index) });
                }
                unsigned difficulty = std::min(5u, hostile_resources_->difficulty);
                if (scan_melee_position(candidates, { uint8_t(ai_config_[0x100 + difficulty]), uint8_t(ai_config_[0x105 + difficulty]) }, attack_selection))
                {
                    return true;
                }
            }
            return false;
        };
        bool stop = scan(origin, 0);
        for (const auto& record : routes.candidates())
        {
            if (stop)
            {
                break;
            }
            if (melee_route_candidate(record.marker, record.category, (actor.status34 & 1) != 0))
            {
                stop = scan(record.position, record.index + 1);
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
        int reach = maximum_route_depth(routes), own_value = hostile_resources_->combat_value(actor);
        bool nearby = nearby_threat(origin, reach, own_value, lookup), ranged = false;
        int kind = actor.appearance && actor.appearance <= ai_kind_classes_.size() ? ai_kind_classes_[actor.appearance - 1] : 0;
        if (!nearby)
        {
            if (auto target = original_target(targets, origin, 3, original_ai::multiply(own_value, 2)))
            {
                const auto& u = units[target->token];
                ranged = target_threat(original_ai::original_distance(origin, target->position), u.base_action_points, u.carry_points, config_[0], config_[6], config_[7], config_[1], hostile_resources_->maximum_action_range(u), reach, false);
            }
        }
        int mode = movement_mode(nearby, ranged);
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
                int distance = original_ai::original_distance(origin, candidate.position);
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
                        int distance = original_ai::original_distance(origin, p);
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
                int d = original_ai::original_distance(r.position, *goal);
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
                return urgent_40CDA1(m, kind, actor.hp, actor.max_hp, actor.mp, actor.max_mp) ? 1 : 0;
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

public:
    void begin_hostile_side(std::vector<OriginalEvents::Unit>& units)
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
    void tick_action_statuses(std::vector<OriginalEvents::Unit>& units)
    {
        for (auto& unit : units)
        {
            if (unit.action_status_runtime)
            {
                auto& bytes = *unit.action_status_runtime;
                bytes[0x33] = uint8_t(unit.status_flags);
                original_combat::tick_status_durations(bytes);
                unit.attack = int(int16_t(unsigned(bytes[0x134]) | (unsigned(bytes[0x135]) << 8)));
                unit.defense = int(int16_t(unsigned(bytes[0x136]) | (unsigned(bytes[0x137]) << 8)));
            }
        }
    }
    void reset_side(std::vector<OriginalEvents::Unit>& units, bool hostile)
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
    // Legacy contact-only test interface; production uses tick_hostile_actions.
    void end_turn(std::vector<OriginalEvents::Unit>& units)
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
    void present_attack(const std::vector<OriginalEvents::Unit>& units, size_t index, const OriginalEvents::ActionRecord* chosen = nullptr)
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
            return;
        }
    }
    bool attack(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot = -1)
    {
        if (a >= units.size() || !units[a].controllable() || !can_attack(units, a, b, slot) || animating_movement())
        {
            return false;
        }
        return strike(units, a, b, slot);
    }
    void physical_feedback(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int damage, bool miss = false)
    {
        reaction_pose_.reset();
        units[b].ending_combat_value = units[b].hp;
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
    int draw_forward_damage(const std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, const OriginalEvents::ActionRecord& action, int attack_style, int* reverse_tactics = nullptr)
    {
        const auto& attacker = units[a];
        const auto& defender = units[b];
        using namespace original_combat;
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
        HitChanceInput hit{ };
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
        last_physical_diagnostic_ = PhysicalDiagnostic{ a, b, std::clamp(attacker.tactics - defender.strikeback + (attack_style == 3 ? 5 : attack_style == 2 ? 3 :
                                                                                                                                                               0),
                                                                  1, 100)
                * 12 / 10,
            hit_chance(hit), hit.modifiers.action_bonus, height, attack_style };
        if (!hit_succeeds(hit, percent())) { return 0; }
        PhysicalDamageInput damage{ };
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
    bool draw_physical_consecutive(const PhysicalActionContext& context, bool reverse)
    {
        const auto& actor = reverse ? context.target_snapshot : context.actor_snapshot;
        const auto& target = reverse ? context.actor_snapshot : context.target_snapshot;
        const auto& resources = *hostile_resources_;
        original_combat::ConsecutiveInput input{ };
        input.has_attack_context = physical_action_preview(resources, actor, target, reverse ? *context.counter : context.action, reverse, context.mode) != 0;
        input.attacker_tactics = actor.tactics;
        input.defender_evasion = target.strikeback;
        input.mode_adjustment = original_combat::consecutive_mode_adjustment(context.mode, reverse);
        input.attacker_equipment_bonus = resources.equipment_accuracy(actor);
        input.defender_equipment_penalty = resources.equipment_evasion(target);
        input.attacker_115 = uint8_t(actor.abilities);
        input.defender_115 = uint8_t(target.abilities);
        input.defender_3c = physical_status(target, 0x3C);
        input.defender_3f = physical_status(target, 0x3F);
        input.defender_equipment_13 = resources.equipment_special(target, 13);
        input.defender_equipment_16 = resources.equipment_special(target, 16);
        return original_combat::consecutive_succeeds(input, percent());
    }
    bool draw_physical_instant(const PhysicalActionContext& context)
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
    Result<std::vector<PhysicalExchangeStep>> try_forward_exchange(const OriginalEvents& resources, const std::vector<OriginalEvents::Unit>& units, size_t actor, size_t primary, const OriginalEvents::ActionRecord& action)
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
    int draw_reverse_damage(const PhysicalActionContext& context)
    {
        using namespace original_combat;
        const auto& actor = context.target_snapshot;
        const auto& target = context.actor_snapshot;
        const auto& action = *context.counter;
        const auto& resources = *hostile_resources_;
        if (!physical_action_preview(resources, actor, target, action, true, context.mode)) { return 0; }
        bool simple = resources.difficulty == 1;
        int height = terrain_height_difference(cell({ actor.x, actor.y }).elevation, cell({ target.x, target.y }).elevation, simple);
        HitChanceInput hit{ };
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
        CounterDamageInput counter{ };
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
    Result<std::vector<PhysicalExchangeStep>> try_counter_exchange(const std::vector<OriginalEvents::Unit>& units, const PhysicalActionContext& original)
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
    static void advance_physical_proficiency(OriginalEvents::Unit& unit, const OriginalEvents::ActionRecord& action)
    {
        int type = action.fields[0];
        unsigned* value = nullptr;
        if (type >= 2 && type <= 7) { value = &unit.weapon_proficiency[size_t(type - 2)]; }
        else if ((type < 1 || type > 10) && type != 11 && type != 12) { value = &unit.special_proficiency; }
        if (value && *value < 1500) { ++*value; }
    }
    void advance_physical_exchange(std::vector<OriginalEvents::Unit>& units)
    {
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
    void rollback_physical_exchange(std::vector<OriginalEvents::Unit>& units)
    {
        if (!physical_exchange_) { return; }
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
        if (physical_transaction_finish_) { physical_transaction_finish_(false); }
    }
    bool strike(std::vector<OriginalEvents::Unit>& units, size_t a, size_t b, int slot = -1)
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
        int hit = std::clamp(attacker.tactics - defender.strikeback + (attack_style == 3 ? 5 : attack_style == 2 ? 3 :
                                                                                                                   0),
                      1, 100)
            * 12 / 10;
        if (percent() > hit)
        {
            physical_feedback(units, a, b, 0, true);
            message_ = "Ordinary melee: miss";
            return true;
        }
        int chance = percent(), jitter = roll(10);
        int divisor = original_combat::damage_divisor(chance, jitter);
        int minimum_roll = (attacker.attack - defender.defense) * 100 / divisor < 3 ? percent() : 0;
        int damage = ordinary_damage(attacker.attack + (attack_style == 3 ? 10 : attack_style == 2 ? 5 :
                                                                                                     0),
            defender.defense, divisor, minimum_roll);
        physical_feedback(units, a, b, damage);
        message_ = "Ordinary melee: -" + std::to_string(damage) + " HP; target HP " + std::to_string(defender.hp);
        return true;
    }
    bool click(std::vector<OriginalEvents::Unit>& units, Point p)
    {
        if (animating_movement() || combat_feedback_active())
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
            if (hit < 0 || !units[hit].controllable() || units[hit].action_state == 2)
            {
                return false;
            }
            return select_unit(units, size_t(hit));
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
            const unsigned movement_class = size_t(selected_) < movement_contexts_.size() ? movement_contexts_[selected_].movement_class : 0;
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
                pose.movement_frame = previous_pose.direction != pose.direction ? 0 : previous_pose.movement_frame + (movement_class == 4 || movement_class == 5 ? 0 : 1);
                movement_poses_.push_back(std::move(pose));
            }
            if (!movement_poses_.empty())
            {
                units[selected_].movement_frame = movement_poses_.back().movement_frame;
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
    bool has_movement_mouse() const { return state_ == State::movement && mouse_position_.x >= 0 && !movement_mouse_[0].empty(); }
    bool draw_movement_mouse(std::vector<uint8_t>& image) const
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
    void draw_finish_directions(std::vector<uint8_t>& image, int left, int top, unsigned direction = 4) const
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
    void draw_projectile(std::vector<uint8_t>& image, int cx, int cy) const
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
    void draw_selection(std::vector<uint8_t>& image, const std::vector<OriginalEvents::Unit>& units, int cx, int cy, int layer = -1, const std::function<void(std::vector<uint8_t>&, Point)>& mask = { }) const
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
};
}    //namespace s2ne
