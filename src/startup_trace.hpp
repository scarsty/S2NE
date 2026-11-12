#pragma once
#include <chrono>
#include <cstdlib>
#include <iostream>
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
    explicit StartupTrace(std::string_view category = "Startup") : category_(category) { }
    void mark(std::string_view name)
    {
        if (!enabled_) { return; }
        const auto now = Clock::now();
        std::cerr << category_ << ' ' << name << " elapsed_ms=" << std::chrono::duration<double, std::milli>(now - start_).count()
                  << " stage_ms=" << std::chrono::duration<double, std::milli>(now - previous_).count() << '\n';
        previous_ = now;
    }
};
}    //namespace s2ne
