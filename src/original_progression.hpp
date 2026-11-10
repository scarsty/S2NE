#pragma once
#include "original_events.hpp"
#include "original_promotion_transaction.hpp"
#include "original_random.hpp"
#include <functional>

namespace s2ne
{
class OriginalProgression
{
    int scale_{ };

public:
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
    Result award_native(OriginalEvents::Unit& unit, OriginalRandom& random, int raw, const OriginalPromotion& promotion, const OriginalRandom::TimeSource& time = { }, int game_mode = 0) const
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
            game_mode);
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
    Result award(OriginalEvents::Unit& unit, OriginalRandom& random, int raw, const Growth& grow, int game_mode = 0) const
    {
        if (game_mode || unit.hostile())
        {
            return { };
        }
        auto staged = unit;
        auto staged_random = random;
        Result result;
        result.experience_award = experience_award(staged, raw);
        if (raw)
        {
            staged.experience = std::min(999u, unsigned(uint16_t(staged.experience + result.experience_award)));
        }
        if (staged.experience >= staged.experience_threshold && staged.level < 99)
        {
            if (!staged.experience_threshold || !grow)
            {
                return { };
            }
            do
            {
                ++staged.level;
                staged.experience = unsigned(uint16_t(staged.experience - staged.experience_threshold));
                if (!grow(staged, staged_random))
                {
                    return { };
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
