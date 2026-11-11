#pragma once
#include "original_events.hpp"
#include "original_script_predicates.hpp"
namespace s2ne
{
// 41C02A's ordered type3/condition2 scan. A positive dialogue count
// requires synchronous41CA6E followed by41FF30; a queue is not equivalent.
class OriginalDeathDispatch
{
public:
    struct Record
    {
        unsigned type{ }, condition{ }, role{ }, lines{ };
    };
    enum class Kind
    {
        none,
        empty_match,
        synchronous_script
    };
    struct Plan
    {
        Kind kind{ Kind::none };
        size_t event{ };
        unsigned native_return{ };
        bool settled() const { return kind != Kind::synchronous_script; }
    };
    static Plan inspect(std::span<const Record> records, unsigned role)
    {
        for (size_t i = 0; i < records.size(); ++i)
        {
            const auto& r = records[i];
            if (r.type != 3 || r.condition != 2 || r.role != role)
            {
                continue;
            }
            // MOV [record+10h],SAR16 is a SIGNED line count. First matching
            // empty record exits the whole scan, not just this iteration.
            if (int16_t(r.lines) <= 0)
            {
                return { Kind::empty_match, i, 0 };
            }
            // Conditions1/3/4 cannot reach41C161 via this dispatcher's filter.
            // The reachable native return is hexadecimal12 (decimal18).
            return { Kind::synchronous_script, i, 0x12 };
        }
        return { };
    }
    // Explicit native call frames:41FF30 ->41C33E ->41B9A3 ->41C33E.
    // No pre-expanded dialogue list; predicates see flags at their call time.
    class Stack
    {
        enum class Routine
        {
            death,
            repeat,
            flags,
            generic
        };
        struct Frame
        {
            Routine routine;
            size_t index{ };
            unsigned pc{ };
            bool changed{ };
        };
        std::vector<Frame> frames_;
        size_t event_count_{ };
        bool returned_change_{ };
        bool evacuation_{ };
        bool supported_{ true };
        size_t completion_{ };
        static bool active(const OriginalEvents& e, size_t i)
        {
            int f = e.event_word(i, 3);
            return f >= 1 && f <= 80 && e.flags[size_t(f - 1)] > 0;
        }
        void complete(OriginalEvents& e, size_t i) { e.complete(i); }

    public:
        bool supported() const { return supported_; }
        bool evacuation_pending() const { return evacuation_; }
        bool finish_evacuation(OriginalEvents& e)
        {
            if (!evacuation_)
            {
                return false;
            }
            e.complete(completion_);
            evacuation_ = false;
            return true;
        }
        size_t depth() const { return frames_.size(); }
        Stack() = default;
        Stack(OriginalEvents& e, size_t death)
        {
            event_count_ = e.event_count();
            if (death >= event_count_) { return; }
            frames_.reserve(event_count_ + 16);
            unsigned f = e.completion_flag(death);
            if (f >= 1 && f <= 80)
            {
                e.flags[f - 1] = 1;
            }
            frames_.push_back({ Routine::death, death });
        }
        std::optional<size_t> next(OriginalEvents& e)
        {
            if (evacuation_)
            {
                return std::nullopt;
            }
            while (!frames_.empty())
            {
                auto& f = frames_.back();
                if (f.routine == Routine::death)
                {
                    if (f.pc++ == 0)
                    {
                        return f.index;
                    }
                    frames_.pop_back();
                    frames_.push_back({ Routine::repeat });
                    continue;
                }
                if (f.routine == Routine::repeat)
                {
                    if (f.pc && !returned_change_)
                    {
                        frames_.pop_back();
                        continue;
                    }
                    f.pc = 1;
                    frames_.push_back({ Routine::flags });
                    continue;
                }
                if (f.index >= event_count_ || f.index >= e.event_count())
                {
                    returned_change_ = f.routine == Routine::flags && f.changed;
                    frames_.pop_back();
                    continue;
                }
                if (f.pc == 0)
                {
                    const unsigned type = e.type(f.index), condition = e.condition(f.index);
                    if (f.routine == Routine::generic && active(e, f.index) && type >= 4 && type <= 16 && !(type == 9 && condition == 0) && !(type == 11 && (condition <= 6 || ((condition == 11 || condition == 12) && e.campaign_currencies))))
                    {
                        supported_ = false;
                        frames_.clear();
                        return std::nullopt;
                    }
                    bool eligible = e.type(f.index) == 9 && e.condition(f.index) == 0 && active(e, f.index);
                    if (f.routine == Routine::generic && active(e, f.index) && e.type(f.index) == 11)
                    {
                        if (auto predicate = original_role_predicate(e, f.index))
                        {
                            eligible = *predicate;
                        }
                        else
                        {
                            eligible = original_currency_handler(e, f.index);
                        }
                    }
                    f.pc = 1;
                    if (eligible)
                    {
                        f.pc = 2;
                        if (int16_t(e.line_count(f.index)) > 0)
                        {
                            return f.index;
                        }
                    }
                }
                if (f.pc == 2)
                {
                    unsigned flag = e.completion_flag(f.index);
                    if (flag == 1 || flag == 3)
                    {
                        supported_ = false;
                        frames_.clear();
                        return std::nullopt;
                    }
                    if (flag == 7)
                    {
                        evacuation_ = true;
                        completion_ = f.index;
                        f.pc = 5;
                        return std::nullopt;
                    }
                    complete(e, f.index);
                    if (f.routine == Routine::flags && flag >= 1 && flag <= 80)
                    {
                        f.changed = true;
                    }
                    f.pc = 3;
                    //41B9A3 calls41C33E only after a successful handler.
                    if (f.routine == Routine::generic)
                    {
                        frames_.push_back({ Routine::flags });
                        continue;
                    }
                }
                if (f.pc == 5)
                {
                    if (f.routine == Routine::flags)
                    {
                        f.changed = true;
                    }
                    f.pc = 3;
                    if (f.routine == Routine::generic)
                    {
                        frames_.push_back({ Routine::flags });
                        continue;
                    }
                }
                if (f.routine == Routine::flags && f.pc != 4)
                {
                    f.pc = 4;
                    frames_.push_back({ Routine::generic });
                    continue;
                }
                ++f.index;
                f.pc = 0;
            }
            return std::nullopt;
        }
    };
    struct Continuation
    {
        bool supported{ };
        std::vector<size_t> dialogue;
    };
    static Continuation continuation(const OriginalEvents& events, unsigned role)
    {
        auto death = inspect(events, role);
        if (death.settled())
        {
            return { true, { } };
        }
        if (events.completion_flag(death.event) == 1 || events.completion_flag(death.event) == 3) { return { }; }
        for (size_t i = 0; i < events.event_count(); ++i)
        {
            unsigned type = events.type(i), condition = events.condition(i);
            // Non-flag handlers have independent native predicates; do not
            // infer their reachability from WORD3 alone.
            if (type >= 4 && type <= 16 && !(type == 9 && condition == 0) && !(type == 11 && (condition <= 6 || condition == 11 || condition == 12))) { return { }; }
        }
        auto staged = events;
        Stack stack(staged, death.event);
        for (;;)
        {
            if (auto i = stack.next(staged))
            {
                for (size_t line = 0; line < staged.line_count(*i); ++line)
                {
                    auto text = staged.try_line(*i, line);
                    if (!text) { return { }; }
                }
            }
            else if (stack.evacuation_pending())
            {
                stack.finish_evacuation(staged);
            }
            else
            {
                break;
            }
        }
        if (!stack.supported()) { return { }; }
        // Only the entry is retained for API compatibility, never the scan order.
        return { true, { death.event } };
    }
    static Plan inspect(const OriginalEvents& events, unsigned role)
    {
        std::vector<Record> records;
        records.reserve(events.event_count());
        for (size_t i = 0; i < events.event_count(); ++i)
        {
            records.push_back({ events.type(i), events.condition(i), unsigned(uint16_t(events.event_word(i, 3))), events.line_count(i) });
        }
        return inspect(records, role);
    }
};
}    //namespace s2ne
