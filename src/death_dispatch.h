#pragma once
#include "events.h"
#include "script_predicates.h"
#include <functional>
namespace s2ne
{
// 41C02A's ordered type3/condition2 scan. A positive dialogue count
// requires synchronous41CA6E followed by41FF30; a queue is not equivalent.
class DeathDispatch
{
public:
    struct Record
    {
        unsigned type{}, condition{}, role{}, lines{};
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
        size_t event{};
        unsigned native_return{};
        bool settled() const;
    };
    static Plan inspect(std::span<const Record> records, unsigned role);
    // Explicit native call frames:41FF30 ->41C33E ->41B9A3 ->41C33E.
    // No pre-expanded dialogue list; predicates see flags at their call time.
    class Stack
    {
    public:
        using AudioHandler = std::function<Result<bool>(const Events&, size_t)>;
        using PresentationHandler = std::function<Result<bool>(Events&, size_t)>;

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
            size_t index{};
            unsigned pc{};
            bool changed{};
        };
        std::vector<Frame> frames_;
        size_t event_count_{};
        bool returned_change_{};
        bool evacuation_{};
        bool supported_{ true };
        size_t completion_{};
        unsigned location_notice_{};
        static bool active(const Events& e, size_t i);
        void complete(Events& e, size_t i);

    public:
        bool supported() const;
        std::optional<size_t> choice_pending() const;
        Result<bool> finish_choice(Events& e, unsigned selected);
        static bool presentation_supported(unsigned type, unsigned condition);
        bool presentation_pending() const;
        bool finish_presentation();
        unsigned location_notice() const;
        bool finish_location_notice(Events&);
        bool evacuation_pending() const;
        bool finish_evacuation(Events& e);
        size_t depth() const;
        Stack() = default;
        Stack(Events& e, size_t death, AudioHandler audio = {}, PresentationHandler presentation = {});
        std::optional<size_t> next(Events& e);
    };
    struct Continuation
    {
        bool supported{};
        std::vector<size_t> dialogue;
    };
    static Continuation continuation(const Events& events, unsigned role, Stack::AudioHandler audio = {}, Stack::PresentationHandler presentation = {});
    static Plan inspect(const Events& events, unsigned role);
};
}    //namespace s2ne
