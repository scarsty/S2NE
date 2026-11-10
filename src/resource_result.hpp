#pragma once
#include "resource_bundle.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <vector>
namespace s2ne
{
inline Result<std::vector<uint8_t>> read_resource(const std::filesystem::path& path)
{
    if (path.filename() == "SUPERSP.PRG")
    {
        return ResourceBundle(path.parent_path()).try_prg_image();
    }
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Cannot open resource: " + path.string()));
    }
    std::vector<uint8_t> bytes{ std::istreambuf_iterator<char>(file), { } };
    if (file.bad())
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Cannot read resource: " + path.string()));
    }
    return Result<std::vector<uint8_t>>::success(std::move(bytes));
}
class ResourceReader
{
    std::span<const uint8_t> bytes_;
    std::optional<Error> error_;

public:
    explicit ResourceReader(std::span<const uint8_t> bytes) : bytes_(bytes) { }
    bool extent(size_t position, size_t count)
    {
        if (position > bytes_.size() || count > bytes_.size() - position)
        {
            reject("Truncated resource field");
            return false;
        }
        return true;
    }
    unsigned number(size_t position, unsigned count = 2)
    {
        if (!extent(position, count))
        {
            return 0;
        }
        unsigned value = 0;
        for (unsigned i = 0; i < count; ++i)
        {
            value |= unsigned(bytes_[position + i]) << (8 * i);
        }
        return value;
    }
    void reject(std::string message)
    {
        if (!error_)
        {
            error_ = make_error(std::move(message));
        }
    }
    explicit operator bool() const { return !error_; }
    const Error& error() const { return *error_; }
};
struct CombatResources
{
    std::vector<uint8_t> bin, pal;
    size_t config{ };
};
inline Result<CombatResources> read_combat_resources(const std::filesystem::path& root)
{
    auto bin = read_resource(root / "SUPERSP.BIN"), pal = read_resource(root / "SUPERSP.PAL");
    if (!bin)
    {
        return Result<CombatResources>::failure(bin.error());
    }
    if (!pal)
    {
        return Result<CombatResources>::failure(pal.error());
    }
    CombatResources result{ std::move(bin).value(), std::move(pal).value(), 0 };
    ResourceReader p(result.pal), b(result.bin);
    unsigned address = p.number(920, 4);
    if (!p)
    {
        return Result<CombatResources>::failure(p.error());
    }
    if (address < 20)
    {
        return Result<CombatResources>::failure(make_error("Invalid combat configuration address"));
    }
    size_t c = address - 20;
    c += 2 + 4 * size_t(b.number(c));
    c += 2 + b.number(c);
    c += 2;
    if (!b.extent(c, 256) || !b)
    {
        return Result<CombatResources>::failure(b.error());
    }
    result.config = c;
    return Result<CombatResources>::success(std::move(result));
}
}    //namespace s2ne
