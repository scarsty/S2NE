#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "runtime_error.h"
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
void invalid_runtime_parameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t);
LONG WINAPI unhandled_runtime_exception(EXCEPTION_POINTERS* exception);
#endif
int report_unexpected_exception() noexcept;
void configure_runtime_diagnostics();
}    // namespace s2ne
