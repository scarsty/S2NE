#pragma once
#include "resource_bundle.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <vector>
namespace s2ne
{
inline unsigned word(const auto& bytes, size_t position) { return unsigned(bytes.at(position)) | (unsigned(bytes.at(position + 1)) << 8); }
inline unsigned dword(const auto& bytes, size_t position) { return word(bytes, position) | (word(bytes, position + 2) << 16); }
Result<std::vector<uint8_t>> read_resource(const std::filesystem::path& path);
Result<SharedBytes> read_shared_resource(const std::filesystem::path& path, ResourceDomain domain = ResourceDomain::primary);
class ResourceReader
{
    std::span<const uint8_t> bytes_;
    std::optional<Error> error_;

public:
    explicit ResourceReader(std::span<const uint8_t> bytes);
    bool extent(size_t position, size_t count);
    unsigned number(size_t position, unsigned count = 2);
    void reject(std::string message);
    explicit operator bool() const;
    const Error& error() const;
};
struct CombatResources
{
    SharedBytes bin, pal;
    size_t config{};
};
Result<CombatResources> read_combat_resources(const std::filesystem::path& root);
}    //namespace s2ne
