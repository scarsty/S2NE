#include "runtime_diagnostics.h"

namespace s2ne
{
#ifdef _WIN32
auto invalid_runtime_parameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) -> void
{
    std::print(stderr, "S2NE: invalid C/C++ runtime parameter; terminating.\n");
    std::fflush(stderr);
    std::_Exit(EXIT_FAILURE);
}

LONG WINAPI unhandled_runtime_exception(EXCEPTION_POINTERS* exception)
{
    std::print(stderr, "S2NE: unhandled Windows exception 0x{:08X} at {}\n", exception->ExceptionRecord->ExceptionCode, static_cast<void*>(exception->ExceptionRecord->ExceptionAddress));
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
auto report_unexpected_exception() noexcept -> int
{
    std::print(stderr, "S2NE: unexpected library C++ exception escaped application; original throw stack unavailable.\n");
    std::fflush(stderr);
    return EXIT_FAILURE;
}

auto configure_runtime_diagnostics() -> void
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
}    //namespace s2ne
