#pragma once
#include "original_ai.h"
#include "original_events.h"
namespace s2ne
{
// 46234F/4391C8(mode2): select again after each presented removal.
class OriginalEvacuation
{
    int x_{}, y_{};
    std::optional<unsigned> target_;
    std::optional<int32_t> saved_delay_;
    bool active_{};

public:
    bool active() const;
    std::optional<unsigned> target() const;
    bool begin(OriginalEvents& e);
    Result<std::optional<unsigned>> try_select(const OriginalEvents& e);
    std::optional<unsigned> select(const OriginalEvents& e);
    bool presented(OriginalEvents& e);
    bool finish(OriginalEvents& e);
};
}    //namespace s2ne
