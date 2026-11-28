#include "progression.h"

namespace s2ne
{
auto Progression::action_reward(const Events::Unit& unit, int raw, unsigned difficulty) const -> int
{
    if (raw && cheat_enabled_ && !unit.npc_class() && !unit.hostile() && !(unit.status_flags & 0x10)) { return 999; }
    int reward = experience_award(unit, raw);
    if (!unit.hostile()) { reward = apply_difficulty_reward(reward, difficulty); }
    return reward;
}

auto Progression::initialize_cheat(const std::filesystem::path& working) -> void { cheat_enabled_ = std::filesystem::is_regular_file(working / "cheat"); }

Progression::PendingGrowth::PendingGrowth(const Events::Unit& unit, RandomStream random, bool enabled) : unit_(unit), random_(random), enabled_(enabled) {}

auto Progression::PendingGrowth::unit() const -> const s2ne::Events::Unit& { return unit_; }

auto Progression::PendingGrowth::random() const -> const s2ne::RandomStream& { return random_; }

auto Progression::PendingGrowth::step() const -> const std::optional<s2ne::Progression::GrowthStep>& { return step_; }

auto Progression::PendingGrowth::failed() const -> bool { return failed_; }

auto Progression::PendingGrowth::learning_name() const -> const std::vector<unsigned char>& { return learning_name_; }

auto Progression::PendingGrowth::experience_notice() const -> int { return experience_notice_; }

auto Progression::PendingGrowth::set_experience_notice(int value) -> void { experience_notice_ = value; }

auto Progression::PendingGrowth::confirm_experience() -> bool
{
    if (experience_notice_ <= 0) { return false; }
    experience_notice_ = 0;
    return true;
}

auto Progression::PendingGrowth::set_learning_name(std::vector<uint8_t> name) -> void { learning_name_ = std::move(name); }

auto Progression::PendingGrowth::confirm_learning() -> bool
{
    if (learning_name_.empty()) { return false; }
    learning_name_.clear();
    return true;
}

auto Progression::PendingGrowth::prepare(const Promotion& promotion, const RandomStream::TimeSource& time) -> bool
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
    auto growth = promotion.try_growth(unit_.appearance, unit_.level, random_, time, Settings::gameplay_assist && !unit_.hostile());
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

auto Progression::PendingGrowth::confirm() -> bool
{
    if (!step_ || failed_) { return false; }
    PromotionTransaction::apply_stat_increments(unit_, step_->increments);
    step_.reset();
    return true;
}

auto Progression::apply_difficulty_reward(int raw, unsigned difficulty) -> int
{
    if (difficulty == 1) { return raw * 3; }
    if (difficulty == 2) { return raw * 2; }
    return raw;
}

auto Progression::stage(const Events::Unit& unit, RandomStream random, int raw, bool hostile_reward, unsigned difficulty, bool present_experience) const -> PendingGrowth
{
    auto staged = unit;
    int reward = action_reward(staged, raw, difficulty);
    if (raw && (!unit.hostile() || hostile_reward)) { staged.experience = std::min(999u, unsigned(uint16_t(staged.experience + reward))); }
    PendingGrowth pending(staged, random, !unit.hostile() || hostile_reward);
    if (present_experience && !unit.hostile() && reward > 0) { pending.set_experience_notice(reward); }
    return pending;
}

auto Progression::create(const std::filesystem::path& root) -> s2ne::Result<Progression>
{
    auto resources = read_combat_resources(root);
    if (!resources)
    {
        return s2ne::Result<Progression>::failure(resources.error());
    }
    auto& data = resources.value();
    Progression result;
    result.scale_ = int(int8_t(data.bin[data.config + 0x10])) + int(int8_t(data.bin[data.config + 0x11])) + 3;
    return s2ne::Result<Progression>::success(std::move(result));
}

Progression::Progression(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto Progression::award_native(Events::Unit& unit, RandomStream& random, int raw, const Promotion& promotion, const RandomStream::TimeSource& time, int game_mode, bool hostile_reward, unsigned difficulty) const -> Result
{
    return award(unit, random, raw, [&](auto& staged, auto& rng)
        {
            auto growth = promotion.try_growth(staged.appearance, staged.level, rng, time, Settings::gameplay_assist && !staged.hostile());
            if (!growth)
            {
                report_error(growth.error());
                return false;
            }
            PromotionTransaction::apply_stat_increments(staged, growth.value());
            return true;
        },
        game_mode, hostile_reward, difficulty);
}

auto Progression::experience_award(const Events::Unit& unit, int raw) const -> int
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

auto Progression::award(Events::Unit& unit, RandomStream& random, int raw, const Growth& grow, int game_mode, bool hostile_reward, unsigned difficulty) const -> Result
{
    if (game_mode || (unit.hostile() && !hostile_reward))
    {
        return {};
    }
    auto staged = unit;
    auto staged_random = random;
    Result result;
    result.experience_award = action_reward(staged, raw, difficulty);
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
}    //namespace s2ne
