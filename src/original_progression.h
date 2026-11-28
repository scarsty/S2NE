#pragma once
#include "original_events.h"
#include "original_promotion_transaction.h"
#include "original_random.h"
#include <functional>

namespace s2ne
{
class OriginalProgression
{
    int scale_{};
    inline static bool cheat_enabled_{};
    int action_reward(const OriginalEvents::Unit& unit, int raw, unsigned difficulty) const;

public:
    static void initialize_cheat(const std::filesystem::path& working);
    struct GrowthStep
    {
        OriginalEvents::Unit before;
        std::array<int, 9> increments{};
    };
    // A single4593CC invocation. RNG and stat writes stay in this candidate
    // across473420; only confirm applies45C3E0 and permits the next draw.
    class PendingGrowth
    {
        OriginalEvents::Unit unit_;
        OriginalRandom random_;
        std::optional<GrowthStep> step_;
        std::vector<uint8_t> learning_name_;
        bool failed_{};
        bool enabled_ = true;
        int experience_notice_{};

    public:
        PendingGrowth(const OriginalEvents::Unit& unit, OriginalRandom random, bool enabled = true);
        const s2ne::OriginalEvents::Unit& unit() const;
        const s2ne::OriginalRandom& random() const;
        const std::optional<s2ne::OriginalProgression::GrowthStep>& step() const;
        bool failed() const;
        const std::vector<unsigned char>& learning_name() const;
        int experience_notice() const;
        void set_experience_notice(int value);
        bool confirm_experience();
        void set_learning_name(std::vector<uint8_t> name);
        bool confirm_learning();
        bool prepare(const OriginalPromotion& promotion, const OriginalRandom::TimeSource& time);
        bool confirm();
    };
    static int apply_difficulty_reward(int raw, unsigned difficulty);
    PendingGrowth stage(const OriginalEvents::Unit& unit, OriginalRandom random, int raw, bool hostile_reward = false, unsigned difficulty = 3, bool present_experience = false) const;
    using Growth = std::function<bool(OriginalEvents::Unit&, OriginalRandom&)>;
    struct Result
    {
        bool applied = false;
        int experience_award = 0;
        unsigned levels = 0;
    };
    OriginalProgression() = default;
    static s2ne::Result<OriginalProgression> create(const std::filesystem::path& root);
    explicit OriginalProgression(const std::filesystem::path& root);
    Result award_native(OriginalEvents::Unit& unit, OriginalRandom& random, int raw, const OriginalPromotion& promotion, const OriginalRandom::TimeSource& time = {}, int game_mode = 0, bool hostile_reward = false, unsigned difficulty = 3) const;
    int experience_award(const OriginalEvents::Unit& unit, int raw) const;
    // 458B03 and 459309/4593CC: publish neither XP nor RNG if growth cannot complete.
    Result award(OriginalEvents::Unit& unit, OriginalRandom& random, int raw, const Growth& grow, int game_mode = 0, bool hostile_reward = false, unsigned difficulty = 3) const;
};
}    //namespace s2ne
