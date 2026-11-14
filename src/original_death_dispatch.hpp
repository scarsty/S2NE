#pragma once
#include "original_events.hpp"
#include "original_script_predicates.hpp"
#include <functional>
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
    public:
        using AudioHandler = std::function<Result<bool>(const OriginalEvents&, size_t)>;
        using PresentationHandler = std::function<Result<bool>(OriginalEvents&, size_t)>;
    private:
        AudioHandler audio_;
        PresentationHandler presentation_;
        bool presentation_pending_{};
        std::optional<size_t> choice_;
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
        unsigned location_notice_{};
        static bool active(const OriginalEvents& e, size_t i)
        {
            int f = e.event_word(i, 3);
            return f >= 1 && f <= 80 && e.flags[size_t(f - 1)] > 0;
        }
        void complete(OriginalEvents& e, size_t i) { e.complete(i); }

    public:
        bool supported() const { return supported_; }
        std::optional<size_t> choice_pending() const { return choice_; }
        Result<bool> finish_choice(OriginalEvents& e, unsigned selected)
        {
            if (!choice_) { return Result<bool>::failure(make_error("No synchronous choice pending")); }
            auto chosen = e.try_choose(*choice_, selected);
            if (!chosen) { return chosen; }
            choice_.reset();
            return Result<bool>::success(true);
        }
        static bool presentation_supported(unsigned type, unsigned condition)
        {
            return (type == 12 && (condition == 10 || condition == 16)) || (type == 7 && condition == 0) || (type == 6 && condition <= 1);
        }
        bool presentation_pending() const { return presentation_pending_; }
        bool finish_presentation()
        {
            if (!presentation_pending_) { return false; }
            presentation_pending_ = false;
            return true;
        }
        unsigned location_notice() const { return location_notice_; }
        bool finish_location_notice(OriginalEvents&)
        {
            if (!location_notice_) { return false; }
            location_notice_ = 0;
            return true;
        }
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
        Stack(OriginalEvents& e, size_t death, AudioHandler audio = {}, PresentationHandler presentation = {}) : audio_(std::move(audio)), presentation_(std::move(presentation))
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
            if (evacuation_ || location_notice_ || presentation_pending_ || choice_)
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
                    if (f.routine == Routine::generic && active(e, f.index) && type >= 4 && type <= 16 && !(type == 5 && condition <= 2) && !(presentation_ && presentation_supported(type, condition)) && !original_death_state_handler_supported(type, condition) && !(type == 9 && condition <= 4) && !(type == 13 && (condition == 1 || condition == 2) && audio_) && !(type == 11 && (condition <= 6 || ((condition == 11 || condition == 12) && e.campaign_currencies))))
                    {
                        supported_ = false;
                        frames_.clear();
                        return std::nullopt;
                    }
                    bool eligible = e.type(f.index) == 9 && e.condition(f.index) == 0 && active(e, f.index);
                    if (f.routine == Routine::generic && active(e, f.index) && type == 5 && condition <= 2)
                    {
                        if (condition == 0)
                        {
                            f.pc = 3;
                            choice_ = f.index;
                            return std::nullopt;
                        }
                        eligible = e.flags[11] == condition;
                        if (eligible)
                        {
                            e.flags[size_t(e.event_word(f.index, 3) - 1)] = 0;
                            e.flags[11] = 0;
                        }
                    }
                    if (f.routine == Routine::generic && active(e, f.index) && presentation_ && presentation_supported(type, condition))
                    {
                        auto presented = presentation_(e, f.index);
                        if (!presented)
                        {
                            report_error(presented.error());
                            supported_ = false;
                            frames_.clear();
                            return std::nullopt;
                        }
                        if (presented.value())
                        {
                            f.pc = 6;
                            presentation_pending_ = true;
                            return std::nullopt;
                        }
                    }
                    if (f.routine == Routine::generic && active(e, f.index) && original_death_state_handler_supported(type, condition))
                    {
                        auto handled = original_death_state_handler(e, f.index);
                        if (!handled) { supported_ = false; frames_.clear(); return std::nullopt; }
                        eligible = handled.value();
                    }
                    if (f.routine == Routine::generic && type == 9 && condition <= 4)
                    {
                        eligible = original_flag_predicate(e, f.index).value();
                    }
                    if (f.routine == Routine::generic && active(e, f.index) && type == 13 && (condition == 1 || condition == 2) && audio_)
                    {
                        auto played = audio_(e, f.index);
                        if (!played)
                        {
                            report_error(played.error());
                            supported_ = false;
                            frames_.clear();
                            return std::nullopt;
                        }
                        eligible = played.value();
                    }
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
                if (f.pc == 6)
                {
                    f.pc = 2;
                    if (int16_t(e.line_count(f.index)) > 0) { return f.index; }
                }
                if (f.pc == 2)
                {
                    unsigned flag = e.completion_flag(f.index);
                    if (flag == 1 || flag == 3)
                    {
                        complete(e, f.index);
                        location_notice_ = flag;
                        completion_ = f.index;
                        f.pc = 5;
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
    static Continuation continuation(const OriginalEvents& events, unsigned role, Stack::AudioHandler audio = {}, Stack::PresentationHandler presentation = {})
    {
        auto death = inspect(events, role);
        if (death.settled())
        {
            return { true, { } };
        }
        // 41FF30按实际旗标进入41B9A3；拒绝由Stack在可达事件处执行，不能扫描无关未激活事件提前拒绝。
        auto staged = events;
        Stack stack(staged, death.event, std::move(audio), std::move(presentation));
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
            else if (auto choice = stack.choice_pending())
            {
                if (staged.line_count(*choice) < 2) { return {}; }
                for (size_t line = 0; line < staged.line_count(*choice); ++line)
                {
                    if (!staged.try_line(*choice, line)) { return {}; }
                }
                // 选择的后续谓词只能在真实输入后决定，不在预检伪选分支。
                break;
            }
            else if (stack.presentation_pending())
            {
                stack.finish_presentation();
            }
            else if (stack.evacuation_pending())
            {
                stack.finish_evacuation(staged);
            }
            else if (stack.location_notice())
            {
                stack.finish_location_notice(staged);
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
