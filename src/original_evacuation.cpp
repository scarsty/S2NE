#include "original_evacuation.h"

namespace s2ne
{
auto OriginalEvacuation::active() const -> bool { return active_; }

auto OriginalEvacuation::target() const -> std::optional<unsigned> { return target_; }

auto OriginalEvacuation::begin(OriginalEvents& e) -> bool
{
    if (active_)
    {
        return false;
    }
    saved_delay_ = e.camera_step_delay;
    e.camera_step_delay = 0;
    active_ = true;
    return true;
}

auto OriginalEvacuation::try_select(const OriginalEvents& e) -> Result<std::optional<unsigned>>
{
    auto selected = target_;
    if (!active_)
    {
        return Result<std::optional<unsigned>>::failure(make_error("Evacuation not active"));
    }
    if (target_)
    {
        return Result<std::optional<unsigned>>::success(target_);
    }
    bool enemy = false;
    int best = 0;
    for (unsigned i = 0; i < e.units.size(); ++i)
    {
        const auto& u = e.units[i];
        enemy = enemy || u.behavior == 2 || u.behavior == 3 || u.behavior == 8;
        if (!enemy || !u.appearance || !(u.status_flags & 0x80) || (u.status_flags & 0x20))
        {
            continue;
        }
        int distance = 0xFFFF;
        if (x_ || y_)
        {
            distance = s2ne::original_distance({ x_, y_ }, { u.x, u.y });
        }
        else
        {
            for (const auto& ally : e.units)
            {
                if (&ally == &u)
                {
                    break;
                }
                if (ally.behavior == 2 || ally.behavior == 3 || ally.behavior == 8)
                {
                    break;
                }
                if (ally.action_state || (ally.status_flags & 0x20) || !(ally.status_flags & 0x80))
                {
                    continue;
                }
                distance = std::min(distance, s2ne::original_distance({ u.x, u.y }, { ally.x, ally.y }));
            }
        }
        if (!selected || distance < best)
        {
            selected = i;
            best = distance;
        }
    }
    target_ = selected;
    if (target_)
    {
        const auto& u = e.units[*target_];
        x_ = u.x;
        y_ = u.y;
    }
    return Result<std::optional<unsigned>>::success(target_);
}

auto OriginalEvacuation::select(const OriginalEvents& e) -> std::optional<unsigned>
{
    auto result = try_select(e);
    if (!result)
    {
        fatal(result.error());
    }
    return result.value();
}

auto OriginalEvacuation::presented(OriginalEvents& e) -> bool
{
    if (!target_ || *target_ >= e.units.size())
    {
        return false;
    }
    e.units.at(*target_).status_flags &= 0x7Fu;
    target_.reset();
    return true;
}

auto OriginalEvacuation::finish(OriginalEvents& e) -> bool
{
    if (target_)
    {
        return false;
    }
    e.camera_step_delay = saved_delay_;
    active_ = false;
    return true;
}
}    //namespace s2ne
