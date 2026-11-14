#pragma once
#include "original_events.hpp"
#include <optional>
namespace s2ne
{
// 原45CDF3先主军后对军，按身份首项返回，不按存活或画面可见性替代。
inline OriginalEvents::Unit* original_script_role(OriginalEvents& events, int id)
{
    for (bool hostile : { false, true })
    {
        for (auto& unit : events.units)
        {
            if (unit.hostile() == hostile && int(unit.id) == id) { return &unit; }
        }
    }
    return nullptr;
}
inline bool original_death_state_handler_supported(unsigned type, unsigned condition)
{
    return (type == 14 && condition <= 1) || (type == 12 && condition == 25) || (type == 9 && condition == 6) || (type == 8 && condition == 8);
}
inline Result<bool> original_death_state_handler(OriginalEvents& events, size_t event)
{
    if (!original_death_state_handler_supported(events.type(event), events.condition(event)))
    {
        return Result<bool>::failure(make_error("Unsupported synchronous state handler"));
    }
    // 4209C4仅重绘战场，420A20返回成功；事件消耗及完成旗标仍由41BA56执行。
    if (events.type(event) == 8) { return Result<bool>::success(true); }
    if (events.type(event) == 9)
    {
        events.script_route = events.event_word(event, 5);
        return Result<bool>::success(true);
    }
    auto* unit = original_script_role(events, events.event_word(event, 5));
    if (!unit) { return Result<bool>::success(false); }
    if (events.type(event) == 14 && events.condition(event) == 1)
    {
        // 原4219EC调用462DD0，对主军关系列累加；不要求角色已在战场登场。
        const int subject = events.event_word(event, 5);
        if (subject < 1 || subject > 40) { return Result<bool>::success(false); }
        for (auto& member : events.units)
        {
            if (!member.hostile() && member.persistent_role6 == 0 && !(member.status_flags & 0x40))
            {
                auto& value = member.relationships[size_t(subject - 1)];
                value = std::clamp(value + events.event_word(event, 7), -128, 127);
            }
        }
        return Result<bool>::success(true);
    }
    if (events.type(event) == 14)
    {
        const int other = events.event_word(event, 7);
        if (other < 1 || other > 40) { return Result<bool>::success(false); }
        auto& relationship = unit->relationships[size_t(other - 1)];
        relationship = std::clamp(relationship + events.event_word(event, 9), -128, 127);
    }
    else if (!(unit->status_flags & 0x20) && (unit->status_flags & 0x80))
    {
        // 原461149对死亡／未出场角色直接成功返回；25仅写WORD+0Ah。
        unit->ai_role10 = events.event_word(event, 9);
        if (unit->action_status_runtime)
        {
            auto& runtime = *unit->action_status_runtime;
            runtime[0x0A] = uint8_t(unit->ai_role10);
            runtime[0x0B] = uint8_t(unsigned(unit->ai_role10) >> 8);
        }
    }
    return Result<bool>::success(true);
}
// 原420A3B的1..4读取WORD+5/+7/+9；2/4仅在谓词成功后清除全部有效操作数。
inline std::optional<bool> original_flag_predicate(OriginalEvents& events, size_t event)
{
    if (events.type(event) != 9 || events.condition(event) > 4) { return std::nullopt; }
    int trigger = events.event_word(event, 3);
    if (trigger < 1 || trigger > 80 || !events.flags[size_t(trigger - 1)]) { return false; }
    unsigned condition = events.condition(event);
    if (!condition) { return true; }
    std::array<int, 3> operands{};
    size_t count = 0;
    for (unsigned offset : { 5u, 7u, 9u })
    {
        int flag = events.event_word(event, offset);
        if (flag >= 1 && flag <= 80) { operands[count++] = flag; }
    }
    if (!count) { return false; }
    bool all = true, any = false;
    for (size_t i = 0; i < count; ++i)
    {
        bool set = events.flags[size_t(operands[i] - 1)] != 0;
        all = all && set;
        any = any || set;
    }
    bool matched = condition <= 2 ? all : any;
    if (matched && (condition == 2 || condition == 4))
    {
        for (size_t i = 0; i < count; ++i) { events.flags[size_t(operands[i] - 1)] = 0; }
    }
    return matched;
}
//4214C1/4214D5: signed word+5 added with native32-bit wrap.
inline bool original_currency_handler(OriginalEvents& events, size_t event)
{
    int condition = events.condition(event);
    if (events.type(event) != 11 || (condition != 11 && condition != 12))
    {
        return false;
    }
    if (!events.campaign_currencies)
    {
        report_error(make_error("Original currency baseline unavailable"));
        return false;
    }
    auto& value = events.campaign_currencies->at(size_t(condition - 11));
    value = int32_t(uint32_t(value) + uint32_t(events.event_word(event, 5)));
    return true;
}
// 4211A3 conditions0..6. False predicates remain pending, not consumed.
inline std::optional<bool> original_role_predicate(const OriginalEvents& events, size_t event)
{
    if (events.type(event) != 11)
    {
        return std::nullopt;
    }
    int condition = events.condition(event), id = events.event_word(event, 5);
    auto allied = std::find_if(events.units.begin(), events.units.end(), [&](const auto& u)
        {
            return !u.hostile() && int(u.id) == id;
        });
    auto any = allied;
    if (any == events.units.end())
    {
        any = std::find_if(events.units.begin(), events.units.end(), [&](const auto& u)
            {
                return u.hostile() && int(u.id) == id;
            });
    }
    if (condition == 0)
    {
        return any != events.units.end() && (any->status_flags & 0x20) != 0;
    }
    if (condition == 1)
    {
        return any != events.units.end() && (any->status_flags & 0x20) == 0;
    }
    // 440D4C(mode0) accepts any matching identity except 40h without80h;
    // it does not test HP, death20h, appearance, or alternate-form10h.
    if (condition == 2)
    {
        for (const auto& u : events.units)
        {
            if (int(u.id) == id && (!(u.status_flags & 0x40) || (u.status_flags & 0x80)))
            {
                return true;
            }
        }
        return false;
    }
    if (condition == 3)
    {
        return allied == events.units.end();
    }
    if (condition == 4)
    {
        return allied != events.units.end() && (allied->status_flags & 0x40) && !(allied->status_flags & 0x80);
    }
    if (condition == 5)
    {
        return allied != events.units.end() && (allied->status_flags & 0x80) != 0;
    }
    if (condition == 6)
    {
        return allied != events.units.end() && !(allied->status_flags & 0x40) && (allied->status_flags & 0x80);
    }
    return std::nullopt;
}
}    //namespace s2ne
