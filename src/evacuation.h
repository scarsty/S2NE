#pragma once
#include "ai.h"
#include "events.h"
namespace s2ne
{
// 46234F/4391C8(mode2): select again after each presented removal.
class Evacuation
{
    int x_{}, y_{};
    std::optional<unsigned> target_;
    std::optional<int32_t> saved_delay_;
    bool active_{};

public:
    bool active() const;
    std::optional<unsigned> target() const;
    bool begin(Events& e);
    Result<std::optional<unsigned>> try_select(const Events& e);
    std::optional<unsigned> select(const Events& e);
    bool presented(Events& e);
    bool finish(Events& e);
};
}    //namespace s2ne
