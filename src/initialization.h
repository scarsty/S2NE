#pragma once
#include "campaign.h"
#include "promotion_transaction.h"
#include "save_bridge.h"
#include "stage_route.h"

namespace s2ne
{
// Original mode0 initialization and runtime arrivals, with caller-owned opening/settings state.
class Initialization
{
    std::filesystem::path root_;
    SharedBytes bin_, pal_, prg_;
    std::vector<uint8_t> items_;
    static unsigned get(const std::vector<uint8_t>& b, size_t p, unsigned n = 2);
    size_t entry(unsigned id) const;
    Result<std::vector<uint8_t>> try_text(unsigned id) const;

public:
    Result<bool> try_describe(Save::State& raw, const Events& events) const;

public:
    struct Source
    {
        std::array<uint8_t, 101> bytes{};
        unsigned group{};
        unsigned id() const;
        int behavior() const;
        int first_turn() const;
        int last_turn() const;
        bool arrives(unsigned turn) const;
        int control_class() const;
        int role6() const;
        int role_a() const;
        int role_c() const;
    };
    struct Opening
    {
        unsigned primary_stage, alternate_stage, opening_stage;
        int currency_a, currency_b;
    };
    struct PlacementMap
    {
        unsigned width{}, height{};
        std::vector<uint8_t> elevations;
        std::vector<std::pair<int, int>> sealed;
        bool accepts(int x, int y) const;
    };
    Result<PlacementMap> try_placement_map(const std::filesystem::path& root, unsigned map) const;
    PlacementMap placement_map(const std::filesystem::path& root, unsigned map) const;
    static std::pair<int, int> place(int x, int y, const PlacementMap& map, std::span<const Save::UnitRecord> first, std::span<const Save::UnitRecord> second);
    Initialization() = default;
    static Result<Initialization> create(const std::filesystem::path& root);
    explicit Initialization(const std::filesystem::path& root);
    Opening opening() const;
    // 44367C/444276: inventory memset, status memset then 44382B's eleven triples.
    Save::Group empty_group() const;
    Result<Source> try_summon_source(unsigned army, unsigned appearance, int level, int x, int y,
        RandomStream& random, const Promotion& promotion, const RandomStream::TimeSource& time = {}) const;
    Result<Save::UnitRecord> try_new_summon_role(const Source& source, unsigned index, unsigned control,
        const Events& events, const PlacementMap& map, std::span<const Save::UnitRecord> first,
        std::span<const Save::UnitRecord> second, unsigned game_mode = 0) const;
    Result<Save::State> try_summon_role(const Save::State& before, const Events& events,
        unsigned army, unsigned slot, int level, int x, int y, unsigned control, RandomStream& random,
        const Promotion& promotion, const RandomStream::TimeSource& time = {}) const;
    Result<std::vector<Source>> try_sources(unsigned stage) const;
    std::vector<Source> sources(unsigned stage) const;
    Result<Save::UnitRecord> try_new_role(const Source& source, const Events::Unit& numeric, unsigned index, const PlacementMap& map,
        std::span<const Save::UnitRecord> first, std::span<const Save::UnitRecord> second) const;
    // 427645 inserts only this turn's arrivals. The caller supplies unchanged source
    // numeric expansion, and publishes the returned copy together with live caches.
    Result<Save::State> try_insert_arrivals(const Save::State& before, const Events& expanded, unsigned turn, const PlacementMap& map, bool enemies_only = false, bool transition_allies = false) const;
    // 42012F condition0: create/reuse matching stage SOURCE roles, not transfer
    // an arbitrary live enemy or grant a replacement with invented statistics.
    Result<Save::State> try_insert_scripted_roles(const Save::State& before, const Events& expanded, size_t event, const PlacementMap& map) const;
    // 461700/46172E ->451DDA. Dead/inactive actors follow 461149.
    Result<Save::State> try_learn_scripted_range(const Save::State& before, const Events& current, size_t event) const;
    // 461535 changes control words in-place, without army transfer or activation.
    // 461236 condition1 withdraws the role (40h on,80h off), not reverse control.
    Result<Save::State> try_mutate_scripted_control(const Save::State& before, const Events& current, size_t event) const;
    // 461215 removes identity from its fixed army slot, not from the vector.
    Result<Save::State> try_reset_scripted_role(const Save::State& before, const Events& current, size_t event) const;
    struct Setup
    {
        // Original4FB488 is DISC.ID selection (0 disc1/1 disc2), NOT renderer backend.
        uint32_t display_backend;
        uint32_t screen_delay{}, speed{ 10 }, music{ 100 }, effects{ 100 }, speech{ 100 };
        std::array<uint8_t, 11> switches{ 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 };
    };
    template <class Settings>
    static Setup setup_from_settings(const Settings& state, uint32_t disc_selection)
    {
        if (disc_selection > 1)
        {
            fatal("Original first-stage disc selection must be0/1");
        }
        Setup setup{ disc_selection };
        setup.screen_delay = uint32_t(state.screen_delay);
        setup.speed = uint32_t(state.message_delay);
        setup.music = uint32_t(state.midi_volume);
        setup.effects = uint32_t(state.cdda_volume);
        setup.speech = uint32_t(state.effect_volume);
        setup.switches = { state.masking, state.flag_4f7d05, state.animation, state.status_window, state.cdda, state.flag_4f7d09, state.flag_4f7d0a, 0, 0, 0, 0 };
        return setup;
    }
    // Boundary: mode0 first battle after427870 arrivals, before first turn dialogue.
    // The supplied flags are the actual completed opening140 snapshot, not a choice guess.
    Result<Save::State> try_fresh(const std::filesystem::path& root, unsigned stage, unsigned difficulty,
        const std::array<uint8_t, 80>& opening_flags, const Setup& setup) const;
    Result<Save::State> try_transition_stage(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const;
    Result<Save::State> try_transition_stage2(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const;
    // Call only when 4443B3 creates a NEW role, never for a 444068 reused role.
    Result<bool> try_grant_source_consumables(Save::Group& group, const Source& source) const;
    void grant_source_consumables(Save::Group& group, const Source& source) const;
    Save::UnitRecord new_role(const Source& source, const Events::Unit& numeric, unsigned index, const PlacementMap& map,
        std::span<const Save::UnitRecord> first, std::span<const Save::UnitRecord> second) const;
    Save::State insert_arrivals(const Save::State& before, const Events& expanded, unsigned turn, const PlacementMap& map, bool enemies_only = false, bool transition_allies = false) const;
    Save::State insert_scripted_roles(const Save::State& before, const Events& expanded, size_t event, const PlacementMap& map) const;
    Save::State learn_scripted_range(const Save::State& before, const Events& current, size_t event) const;
    Save::State mutate_scripted_control(const Save::State& before, const Events& current, size_t event) const;
    Save::State reset_scripted_role(const Save::State& before, const Events& current, size_t event) const;
    Save::State fresh(const std::filesystem::path& root, unsigned stage, unsigned difficulty,
        const std::array<uint8_t, 80>& opening_flags, const Setup& setup) const;
    Save::State transition_stage(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const;
    Save::State transition_stage2(const std::filesystem::path& root, const Save::State& settled, const Events& deployed) const;
};
}    //namespace s2ne
