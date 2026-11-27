#pragma once
#include <cstdio>
#include <cstdlib>
#include <print>
#ifdef __ANDROID__
#include <android/log.h>
#endif
#if __has_include(<stacktrace>)
#include <stacktrace>
#endif
#if !defined(__cpp_lib_stacktrace) || __cpp_lib_stacktrace < 202011L
#include <array>
#include <cstdint>
#include <dlfcn.h>
#include <unwind.h>
#endif
#include <string>
#include <utility>
#include <variant>
namespace s2ne
{
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
using StackTrace = std::stacktrace;
inline StackTrace capture_stack() noexcept { return std::stacktrace::current(1); }
inline void print_stack(const StackTrace& stack) noexcept
{
    for (size_t i = 0; i < stack.size(); ++i)
    {
        const auto description = stack[i].description();
        const auto file = stack[i].source_file();
        std::print(stderr, "  #{} {} ({}:{})\n", i, description, file, static_cast<unsigned long>(stack[i].source_line()));
    }
    std::fflush(stderr);
}
#else
struct StackTrace
{
    std::array<uintptr_t, 64> frames{};
    size_t count{};
};
inline StackTrace capture_stack() noexcept
{
    StackTrace stack;
    _Unwind_Backtrace([](_Unwind_Context* context, void* argument) -> _Unwind_Reason_Code
        {
            auto& trace = *static_cast<StackTrace*>(argument);
            const auto address = _Unwind_GetIP(context);
            if (address != 0) { trace.frames[trace.count++] = address; }
            return trace.count == trace.frames.size() ? _URC_END_OF_STACK : _URC_NO_REASON;
        },
        &stack);
    return stack;
}
inline void print_stack(const StackTrace& stack) noexcept
{
    for (size_t i = 1; i < stack.count; ++i)
    {
        const auto address = stack.frames[i];
        Dl_info info{};
        if (dladdr(reinterpret_cast<const void*>(address - 1), &info) != 0)
        {
            const char* symbol = info.dli_sname ? info.dli_sname : "<unknown>";
            const char* module = info.dli_fname ? info.dli_fname : "<unknown>";
            const auto base = info.dli_saddr ? info.dli_saddr : info.dli_fbase;
            std::print(stderr, "  #{} {}+0x{:x} ({}+0x{:x}) [{}]\n", i - 1, symbol,
                address - reinterpret_cast<uintptr_t>(base), module,
                address - reinterpret_cast<uintptr_t>(info.dli_fbase), reinterpret_cast<const void*>(address));
        }
        else
        {
            std::print(stderr, "  #{} <unknown> [{}]\n", i - 1, reinterpret_cast<const void*>(address));
        }
    }
    std::fflush(stderr);
}
#endif
struct Error
{
    std::string message;
};
inline Error make_error(std::string message) { return { std::move(message) }; }
inline void report_error(const Error& error) noexcept
{
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_ERROR, "S2NE", error.message.c_str());
#endif
    std::print(stderr, "S2NE: {}\n", error.message);
}
[[noreturn]] inline void fatal(const Error& error) noexcept
{
    report_error(error);
    std::_Exit(EXIT_FAILURE);
}
[[noreturn]] inline void fatal(std::string message) { fatal(make_error(std::move(message))); }
template <class T>
class Result
{
    std::variant<T, Error> state;
    explicit Result(T value) : state(std::in_place_index<0>, std::move(value)) {}
    explicit Result(Error error) : state(std::in_place_index<1>, std::move(error)) {}

public:
    static Result success(T value) { return Result(std::move(value)); }
    static Result failure(Error error) { return Result(std::move(error)); }
    explicit operator bool() const noexcept { return state.index() == 0; }
    T& value() &
    {
        if (!*this)
        {
            fatal(error());
        }
        return std::get<0>(state);
    }
    const T& value() const&
    {
        if (!*this)
        {
            fatal(error());
        }
        return std::get<0>(state);
    }
    T&& value() &&
    {
        if (!*this)
        {
            fatal(error());
        }
        return std::get<0>(std::move(state));
    }
    const Error& error() const
    {
        if (*this)
        {
            fatal("Result error requested on success");
        }
        return std::get<1>(state);
    }
};
}    //namespace s2ne
