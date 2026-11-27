#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "runtime_error.hpp"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <print>
#ifdef _WIN32
#include <crtdbg.h>
#include <eh.h>
#include <windows.h>
#endif

namespace s2ne
{
#ifdef _WIN32
inline void invalid_runtime_parameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
    std::print(stderr, "S2NE: invalid C/C++ runtime parameter; terminating.\n");
    std::fflush(stderr);
    std::_Exit(EXIT_FAILURE);
}
inline LONG WINAPI unhandled_runtime_exception(EXCEPTION_POINTERS* exception)
{
    std::print(stderr, "S2NE: unhandled Windows exception 0x{:08X} at {}\n", exception->ExceptionRecord->ExceptionCode, static_cast<void*>(exception->ExceptionRecord->ExceptionAddress));
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
inline int report_unexpected_exception() noexcept
{
    std::print(stderr, "S2NE: unexpected library C++ exception escaped application; original throw stack unavailable.\n");
    std::fflush(stderr);
    return EXIT_FAILURE;
}
inline void configure_runtime_diagnostics()
{
#ifdef _WIN32
    SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_invalid_parameter_handler(invalid_runtime_parameter);
#else
    std::signal(SIGPIPE, SIG_IGN);
#endif
    std::signal(SIGABRT, [](int)
        {
            std::print(stderr, "S2NE: runtime abort/assertion failure; terminating.\n");
            std::fflush(stderr);
            std::_Exit(EXIT_FAILURE);
        });
#ifdef _WIN32
    _set_purecall_handler([]
        {
            std::print(stderr, "S2NE: pure virtual function call; terminating.\n");
            std::fflush(stderr);
            std::_Exit(EXIT_FAILURE);
        });
#endif
    std::set_terminate([]
        {
            std::print(stderr, "S2NE: unhandled C++ exception or terminate request.\n");
            std::fflush(stderr);
            std::_Exit(EXIT_FAILURE);
        });
#ifdef _WIN32
    SetUnhandledExceptionFilter(unhandled_runtime_exception);
#ifdef _DEBUG
    _CrtSetReportHook([](int, char*, int*) -> int
        {
            return FALSE;
        });
    for (int type : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT })
    {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
#endif
#endif
}
}    // namespace s2ne
