#pragma once
#include "original_events.hpp"
#include "original_promotion_transaction.hpp"
#include "original_random.hpp"
#include <functional>

namespace s2ne
{
class OriginalProgression
{
    int scale_{};

public:
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

    public:
        PendingGrowth(const OriginalEvents::Unit& unit, OriginalRandom random, bool enabled = true) : unit_(unit), random_(random), enabled_(enabled) {}
        const auto& unit() const { return unit_; }
        const auto& random() const { return random_; }
        const auto& step() const { return step_; }
        bool failed() const { return failed_; }
        const auto& learning_name() const { return learning_name_; }
        void set_learning_name(std::vector<uint8_t> name) { learning_name_ = std::move(name); }
        bool confirm_learning()
        {
            if (learning_name_.empty()) { return false; }
            learning_name_.clear();
            return true;
        }
        bool prepare(const OriginalPromotion& promotion, const OriginalRandom::TimeSource& time)
        {
            if (step_ || failed_) { return !failed_; }
            if (!enabled_) { return true; }
            if (unit_.experience < unit_.experience_threshold || unit_.level >= 99) { return true; }
            if (!unit_.experience_threshold)
            {
                failed_ = true;
                return false;
            }
            ++unit_.level;
            auto growth = promotion.try_growth(unit_.appearance, unit_.level, random_, time);
            if (!growth)
            {
                failed_ = true;
                report_error(growth.error());
                return false;
            }
            unit_.experience = unsigned(uint16_t(unit_.experience - unit_.experience_threshold));
            step_ = GrowthStep{ unit_, growth.value() };
            return true;
        }
        bool confirm()
        {
            if (!step_ || failed_) { return false; }
            OriginalPromotionTransaction::apply_stat_increments(unit_, step_->increments);
            step_.reset();
            return true;
        }
    };
    static int apply_difficulty_reward(int raw, unsigned difficulty)
    {
        if (difficulty == 1) { return raw * 3; }
        if (difficulty == 2) { return raw * 2; }
        return raw;
    }
    PendingGrowth stage(const OriginalEvents::Unit& unit, OriginalRandom random, int raw, bool hostile_reward = false, unsigned difficulty = 3) const
    {
        auto staged = unit;
        int reward = experience_award(staged, raw);
        if (!unit.hostile()) { reward = apply_difficulty_reward(reward, difficulty); }
        if (raw && (!unit.hostile() || hostile_reward)) { staged.experience = std::min(999u, unsigned(uint16_t(staged.experience + reward))); }
        return PendingGrowth(staged, random, !unit.hostile() || hostile_reward);
    }
    using Growth = std::function<bool(OriginalEvents::Unit&, OriginalRandom&)>;
    struct Result
    {
        bool applied = false;
        int experience_award = 0;
        unsigned levels = 0;
    };
    OriginalProgression() = default;
    static s2ne::Result<OriginalProgression> create(const std::filesystem::path& root)
    {
        auto resources = read_combat_resources(root);
        if (!resources)
        {
            return s2ne::Result<OriginalProgression>::failure(resources.error());
        }
        auto& data = resources.value();
        OriginalProgression result;
        result.scale_ = int(int8_t(data.bin[data.config + 0x10])) + int(int8_t(data.bin[data.config + 0x11])) + 3;
        return s2ne::Result<OriginalProgression>::success(std::move(result));
    }
    explicit OriginalProgression(const std::filesystem::path& root)
    {
        auto result = create(root);
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    Result award_native(OriginalEvents::Unit& unit, OriginalRandom& random, int raw, const OriginalPromotion& promotion, const OriginalRandom::TimeSource& time = {}, int game_mode = 0, bool hostile_reward = false, unsigned difficulty = 3) const
    {
        return award(unit, random, raw, [&](auto& staged, auto& rng)
            {
                auto growth = promotion.try_growth(staged.appearance, staged.level, rng, time);
                if (!growth)
                {
                    report_error(growth.error());
                    return false;
                }
                OriginalPromotionTransaction::apply_stat_increments(staged, growth.value());
                return true;
            },
            game_mode, hostile_reward, difficulty);
    }
    int experience_award(const OriginalEvents::Unit& unit, int raw) const
    {
        if (!raw)
        {
            return 0;
        }
        if (unit.abilities & 0x40u)
        {
            raw += raw / 3;
        }
        return std::clamp(raw * scale_ / 10, 1, 999);
    }
    // 458B03 and 459309/4593CC: publish neither XP nor RNG if growth cannot complete.
    Result award(OriginalEvents::Unit& unit, OriginalRandom& random, int raw, const Growth& grow, int game_mode = 0, bool hostile_reward = false, unsigned difficulty = 3) const
    {
        if (game_mode || (unit.hostile() && !hostile_reward))
        {
            return {};
        }
        auto staged = unit;
        auto staged_random = random;
        Result result;
        result.experience_award = experience_award(staged, raw);
        if (!unit.hostile()) { result.experience_award = apply_difficulty_reward(result.experience_award, difficulty); }
        if (raw)
        {
            staged.experience = std::min(999u, unsigned(uint16_t(staged.experience + result.experience_award)));
        }
        if (staged.experience >= staged.experience_threshold && staged.level < 99)
        {
            if (!staged.experience_threshold || !grow)
            {
                return {};
            }
            do
            {
                ++staged.level;
                staged.experience = unsigned(uint16_t(staged.experience - staged.experience_threshold));
                if (!grow(staged, staged_random))
                {
                    return {};
                }
                ++result.levels;
            } while (staged.experience >= staged.experience_threshold && staged.level < 99);
        }
        unit = std::move(staged);
        random = std::move(staged_random);
        result.applied = true;
        return result;
    }
};
}    //namespace s2ne
