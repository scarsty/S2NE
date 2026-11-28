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
StackTrace capture_stack() noexcept;
void print_stack(const StackTrace& stack) noexcept;
#else
struct StackTrace
{
    std::array<uintptr_t, 64> frames{};
    size_t count{};
};
StackTrace capture_stack() noexcept;
void print_stack(const StackTrace& stack) noexcept;
#endif
struct Error
{
    std::string message;
};
Error make_error(std::string message);
void report_error(const Error& error) noexcept;
[[noreturn]] void fatal(const Error& error) noexcept;
[[noreturn]] void fatal(std::string message);
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
