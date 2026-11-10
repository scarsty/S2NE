#pragma once
#include "original_ai.hpp"
#include "original_events.hpp"
namespace s2ne
{
// 46234F/4391C8(mode2): select again after each presented removal.
class OriginalEvacuation
{
    int x_{ }, y_{ };
    std::optional<unsigned> target_;
    std::optional<int32_t> saved_delay_;
    bool active_{ };

public:
    bool active() const { return active_; }
    std::optional<unsigned> target() const { return target_; }
    bool begin(OriginalEvents& e)
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
    Result<std::optional<unsigned>> try_select(const OriginalEvents& e)
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
                distance = original_ai::original_distance({ x_, y_ }, { u.x, u.y });
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
                    distance = std::min(distance, original_ai::original_distance({ u.x, u.y }, { ally.x, ally.y }));
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
    std::optional<unsigned> select(const OriginalEvents& e)
    {
        auto result = try_select(e);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }
    bool presented(OriginalEvents& e)
    {
        if (!target_ || *target_ >= e.units.size())
        {
            return false;
        }
        e.units.at(*target_).status_flags &= 0x7Fu;
        target_.reset();
        return true;
    }
    bool finish(OriginalEvents& e)
    {
        if (target_)
        {
            return false;
        }
        e.camera_step_delay = saved_delay_;
        active_ = false;
        return true;
    }
};
}    //namespace s2ne
