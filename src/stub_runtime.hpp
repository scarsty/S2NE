#pragma once
#include "runtime_error.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace s2ne
{
// Watcom register calling conventions are not yet reconstructed per function.
// This is an analysis interface, not a binary-compatible native signature.
struct X86Context
{
    std::uint32_t eax{ }, ebx{ }, ecx{ }, edx{ }, esi{ }, edi{ }, ebp{ }, esp{ }, eflags{ };
};
[[noreturn]] inline void unimplemented_function(std::uint32_t address)
{
    char text[96];
    std::snprintf(text, sizeof(text), "Original function 0x%08X is not implemented", address);
    fatal(make_error(text));
}
using Stub = void (*)(X86Context&);
struct FunctionRecord
{
    std::uint32_t address;
    std::uint32_t end_address;
    std::uint32_t ida_flags;
    const char* original_name;
    Stub stub;
};
extern const FunctionRecord functions[];
extern const std::size_t function_count;
}    //namespace s2ne
