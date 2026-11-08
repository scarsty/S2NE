#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace s2ne {
// Watcom register calling conventions are not yet reconstructed per function.
// This is an analysis interface, not a binary-compatible native signature.
struct X86Context {
    std::uint32_t eax{}, ebx{}, ecx{}, edx{}, esi{}, edi{}, ebp{}, esp{}, eflags{};
};
class UnimplementedFunction : public std::logic_error {
public:
    explicit UnimplementedFunction(std::uint32_t address)
        : std::logic_error("Original function is not implemented"), address(address) {}
    const std::uint32_t address;
};
using Stub = void (*)(X86Context&);
struct FunctionRecord {
    std::uint32_t address;
    std::uint32_t end_address;
    std::uint32_t ida_flags;
    const char* original_name;
    Stub stub;
};
extern const FunctionRecord functions[];
extern const std::size_t function_count;
}
