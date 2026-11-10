#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "runtime_error.hpp"
#include <crtdbg.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <eh.h>
#include <exception>
#include <initializer_list>
#include <windows.h>

namespace s2ne
{
inline void invalid_runtime_parameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
    std::fputs("S2NE: invalid C/C++ runtime parameter; terminating.\n", stderr);
    print_stack(capture_stack());
    std::fflush(stderr);
    std::_Exit(EXIT_FAILURE);
}
inline LONG WINAPI unhandled_runtime_exception(EXCEPTION_POINTERS* exception)
{
    std::fprintf(stderr, "S2NE: unhandled Windows exception 0x%08lX at %p\n", exception->ExceptionRecord->ExceptionCode, exception->ExceptionRecord->ExceptionAddress);
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
inline int report_unexpected_exception() noexcept
{
    std::fputs("S2NE: unexpected library C++ exception escaped application; original throw stack unavailable.\n", stderr);
    std::fflush(stderr);
    return EXIT_FAILURE;
}
inline void configure_runtime_diagnostics()
{
    SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_invalid_parameter_handler(invalid_runtime_parameter);
    std::signal(SIGABRT, [](int)
        {
            std::fputs("S2NE: runtime abort/assertion failure; terminating.\n", stderr);
            print_stack(capture_stack());
            std::fflush(stderr);
            std::_Exit(EXIT_FAILURE);
        });
    _set_purecall_handler([]
        {
            std::fputs("S2NE: pure virtual function call; terminating.\n", stderr);
            print_stack(capture_stack());
            std::fflush(stderr);
            std::_Exit(EXIT_FAILURE);
        });
    std::set_terminate([]
        {
            std::fputs("S2NE: unhandled C++ exception or terminate request.\n", stderr);
            print_stack(capture_stack());
            std::fflush(stderr);
            std::_Exit(EXIT_FAILURE);
        });
    SetUnhandledExceptionFilter(unhandled_runtime_exception);
#ifdef _DEBUG
    _CrtSetReportHook([](int, char*, int*) -> int
        {
            print_stack(capture_stack());
            return FALSE;
        });
    for (int type : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT })
    {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
#endif
}
}    // namespace s2ne
