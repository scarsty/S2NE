#pragma once
#include <cstdio>
#include <cstdlib>
#include <stacktrace>
#include <string>
#include <utility>
#include <variant>
namespace s2ne
{
using StackTrace = std::stacktrace;
inline StackTrace capture_stack() noexcept { return std::stacktrace::current(1); }
inline void print_stack(const StackTrace& stack) noexcept
{
    for (size_t i = 0; i < stack.size(); ++i)
    {
        const auto description = stack[i].description();
        const auto file = stack[i].source_file();
        std::fprintf(stderr, "  #%zu %s (%s:%lu)\n", i, description.c_str(), file.c_str(), static_cast<unsigned long>(stack[i].source_line()));
    }
    std::fflush(stderr);
}
struct Error
{
    std::string message;
    StackTrace stack;
};
inline Error make_error(std::string message) { return { std::move(message), std::stacktrace::current(1) }; }
inline void report_error(const Error& error) noexcept
{
    std::fprintf(stderr, "S2NE: %s\n", error.message.c_str());
    print_stack(error.stack);
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
    explicit Result(T value) : state(std::in_place_index<0>, std::move(value)) { }
    explicit Result(Error error) : state(std::in_place_index<1>, std::move(error)) { }

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
