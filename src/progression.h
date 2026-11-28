#pragma once
#include "events.h"
#include "promotion_transaction.h"
#include "random_stream.h"
#include <functional>

namespace s2ne
{
class Progression
{
    int scale_{};
    inline static bool cheat_enabled_{};
    int action_reward(const Events::Unit& unit, int raw, unsigned difficulty) const;

public:
    static void initialize_cheat(const std::filesystem::path& working);
    struct GrowthStep
    {
        Events::Unit before;
        std::array<int, 9> increments{};
    };
    // A single4593CC invocation. RNG and stat writes stay in this candidate
    // across473420; only confirm applies45C3E0 and permits the next draw.
    class PendingGrowth
    {
        Events::Unit unit_;
        RandomStream random_;
        std::optional<GrowthStep> step_;
        std::vector<uint8_t> learning_name_;
        bool failed_{};
        bool enabled_ = true;
        int experience_notice_{};

    public:
        PendingGrowth(const Events::Unit& unit, RandomStream random, bool enabled = true);
        const s2ne::Events::Unit& unit() const;
        const s2ne::RandomStream& random() const;
        const std::optional<s2ne::Progression::GrowthStep>& step() const;
        bool failed() const;
        const std::vector<unsigned char>& learning_name() const;
        int experience_notice() const;
        void set_experience_notice(int value);
        bool confirm_experience();
        void set_learning_name(std::vector<uint8_t> name);
        bool confirm_learning();
        bool prepare(const Promotion& promotion, const RandomStream::TimeSource& time);
        bool confirm();
    };
    static int apply_difficulty_reward(int raw, unsigned difficulty);
    PendingGrowth stage(const Events::Unit& unit, RandomStream random, int raw, bool hostile_reward = false, unsigned difficulty = 3, bool present_experience = false) const;
    using Growth = std::function<bool(Events::Unit&, RandomStream&)>;
    struct Result
    {
        bool applied = false;
        int experience_award = 0;
        unsigned levels = 0;
    };
    Progression() = default;
    static s2ne::Result<Progression> create(const std::filesystem::path& root);
    explicit Progression(const std::filesystem::path& root);
    Result award_native(Events::Unit& unit, RandomStream& random, int raw, const Promotion& promotion, const RandomStream::TimeSource& time = {}, int game_mode = 0, bool hostile_reward = false, unsigned difficulty = 3) const;
    int experience_award(const Events::Unit& unit, int raw) const;
    // 458B03 and 459309/4593CC: publish neither XP nor RNG if growth cannot complete.
    Result award(Events::Unit& unit, RandomStream& random, int raw, const Growth& grow, int game_mode = 0, bool hostile_reward = false, unsigned difficulty = 3) const;
};
}    //namespace s2ne
