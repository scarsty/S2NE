#include "runtime_error.h"

namespace s2ne
{
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
auto capture_stack() noexcept -> StackTrace { return std::stacktrace::current(1); }

auto print_stack(const StackTrace& stack) noexcept -> void
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
StackTrace capture_stack() noexcept
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
void print_stack(const StackTrace& stack) noexcept
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
auto make_error(std::string message) -> Error { return { std::move(message) }; }

auto report_error(const Error& error) noexcept -> void
{
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_ERROR, "S2NE", error.message.c_str());
#endif
    std::print(stderr, "S2NE: {}\n", error.message);
}

auto fatal(const Error& error) noexcept -> void
{
    report_error(error);
    std::_Exit(EXIT_FAILURE);
}

auto fatal(std::string message) -> void { fatal(make_error(std::move(message))); }
}    //namespace s2ne
