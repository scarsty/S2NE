#pragma once
#include "terminal_output.h"
#include <chrono>
#include <cstdlib>
#include <string_view>
namespace s2ne
{
class StartupTrace
{
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_ = Clock::now(), previous_ = start_;
    bool enabled_ = std::getenv("S2NE_STARTUP_TRACE") != nullptr;
    std::string_view category_;

public:
    explicit StartupTrace(std::string_view category = "Startup") : category_(category) {}
    void mark(std::string_view name)
    {
        if (!enabled_) { return; }
        const auto now = Clock::now();
        std::println(stderr, "{} {} elapsed_ms={} stage_ms={}", category_, name,
            std::chrono::duration<double, std::milli>(now - start_).count(),
            std::chrono::duration<double, std::milli>(now - previous_).count());
        previous_ = now;
    }
};
}    //namespace s2ne
