#pragma once
#include "events.h"
#include "menu.h"
#include "script_predicates.h"
#include "sprite.h"
#include "stage_route.h"
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>

namespace s2ne
{
// Original 438381: dispatch pending events before calling evaluate(), and never
// evaluate while movement, dialogue, combat feedback or death settlement is active.
class Campaign
{
    std::filesystem::path root_;
    SharedBytes bin_, pal_, cel_, prg_;
    std::vector<uint8_t> items_;
    Menu font_;
    size_t table() const;
    s2ne::Result<size_t> try_header(unsigned stage) const;
    size_t header(unsigned stage) const;
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned color) const;

public:
    enum class Outcome
    {
        pending,
        victory,
        defeat
    };
    struct Reward
    {
        unsigned type{};
        std::vector<int> values;
    };
    struct Stage
    {
        unsigned id{}, next{}, mode{};
        std::array<std::vector<int>, 2> victory;
        std::vector<int> defeat;
        std::vector<Reward> rewards;
        std::vector<uint8_t> name;
    };
    struct Runtime
    {
        unsigned turn{ 1 }, mode{};
        bool idle{ true }, forced_victory{}, forced_defeat{};
        // Original predicates 1..20 and 31/32 need map-object/withdrawal state.
        std::function<bool(int)> special_condition;
    };
    struct Decision
    {
        Outcome outcome{ Outcome::pending };
        unsigned group{};
    };
    struct Settlement
    {
        unsigned stage{}, next{}, group{};
        std::vector<Reward> rewards;
        std::vector<size_t> survivor_indices;
    };
    Campaign() = default;
    static Result<Campaign> create(const std::filesystem::path& root);
    explicit Campaign(const std::filesystem::path& root);
    Result<Stage> try_stage(unsigned id) const;
    Stage stage(unsigned id) const;
    struct StoryAnimationRole
    {
        unsigned id{};
        int direction{}, action{};
        size_t event{};
    };
    struct StoryAnimationGroup
    {
        bool hostile{};
        size_t begin{}, end{};
        std::vector<StoryAnimationRole> roles;
    };
    static s2ne::Result<StoryAnimationGroup> try_story_animation_group(const Events& events, size_t begin);
    static StoryAnimationGroup story_animation_group(const Events& events, size_t begin);
    struct StoryMovementRole
    {
        unsigned id{};
        int x{}, y{};
        size_t event{};
    };
    struct StoryMovementGroup
    {
        size_t begin{}, end{};
        bool hostile{};
        unsigned camera{};
        std::vector<StoryMovementRole> roles;
    };
    static s2ne::Result<StoryMovementGroup> try_story_movement_group(const Events& events, size_t event);
    static StoryMovementGroup story_movement_group(const Events& events, size_t event);
    static s2ne::Result<unsigned> try_completed_story_route(const Events& source, const Events& completed, unsigned header_next = 0);
    static unsigned completed_story_route(const Events& source, const Events& completed, unsigned header_next = 0);
    unsigned selected_route(const Events& completed) const;
    s2ne::Result<unsigned> try_animation_sequence_length(unsigned appearance, unsigned sequence) const;
    unsigned animation_sequence_length(unsigned appearance, unsigned sequence) const;
    s2ne::Result<bool> try_protagonist_at_exit(const Events& events) const;
    bool protagonist_at_exit(const Events& events) const;
    static bool living(const Events::Unit& u);
    static bool absent(std::span<const Events::Unit> roster, int selector);
    static Decision evaluate(const Stage& s, std::span<const Events::Unit> allies,
        std::span<const Events::Unit> enemies, const Runtime& rt);
    static std::vector<size_t> victory_events(const Events& events, unsigned group);
    static s2ne::Result<Settlement> try_plan(const Stage& s, Decision d, std::span<const Events::Unit> allies);
    static Settlement plan(const Stage& s, Decision d, std::span<const Events::Unit> allies);
    enum class SavePoint
    {
        before_stage_story,
        before_battle,
        rotate_previous
    };
    struct SaveRequest
    {
        SavePoint point;
        unsigned slot;
        bool rotate_to_previous;
    };
    static std::optional<SaveRequest> save_request(SavePoint point, unsigned original_mode, unsigned turn);
    // 436138/436170 request only; never write original SAVE fixtures here.
    static std::optional<unsigned> autosave_slot(unsigned original_mode, unsigned turn, bool before_story);
    // Type1 recruit, type2 departure, type3 money, type4 item.
    // Adapter stages all original effects on a copy and commits once; failed or
    // unsupported effects cannot leave a partially rewarded live campaign.
    template <class State, class Apply>
    static bool transact(State& live, const Settlement& p, Apply apply)
    {
        State next = live;
        if (!apply(next, p))
        {
            return false;
        }
        live = std::move(next);
        return true;
    }
    static unsigned victory_experience(unsigned xp);
    // Verified 46243A reward panel subset (recruit/money/items); not the 410C86 hub.
    // Call only after reward effects are staged, and await confirm before +30 XP.
    s2ne::Result<bool> try_render_rewards_validated(std::vector<uint8_t>& image, const Stage& s,
        std::span<const Events::Unit> roster) const;
    s2ne::Result<bool> try_render_rewards(std::vector<uint8_t>& image, const Stage& s,
        std::span<const Events::Unit> roster) const;
    bool render_rewards(std::vector<uint8_t>& image, const Stage& s,
        std::span<const Events::Unit> roster) const;
};
}    //namespace s2ne
