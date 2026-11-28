#pragma once
#include "runtime_error.h"
#include "startup_trace.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>
namespace s2ne
{
class SharedBytes
{
    std::shared_ptr<std::vector<uint8_t>> bytes_;
    const std::vector<uint8_t>& view() const;

public:
    SharedBytes() = default;
    explicit SharedBytes(std::vector<uint8_t> bytes);
    explicit SharedBytes(std::shared_ptr<std::vector<uint8_t>> bytes);
    size_t size() const;
    bool empty() const;
    const uint8_t* data() const;
    uint8_t* mutable_data();
    const uint8_t& at(size_t index) const;
    uint8_t& mutable_at(size_t index);
    const uint8_t& operator[](size_t index) const;
    std::vector<uint8_t>::const_iterator begin() const;
    std::vector<uint8_t>::const_iterator end() const;
    void assign(std::vector<uint8_t>::const_iterator first, std::vector<uint8_t>::const_iterator last);
    operator std::span<const uint8_t>() const;
    operator const std::vector<uint8_t>&() const;
    void detach();
};

Result<SharedBytes> load_shared_resource(const std::filesystem::path& path, const std::string& discriminator, const std::function<Result<std::vector<uint8_t>>()>& loader);
SharedBytes share_resource(const std::filesystem::path& path, std::vector<uint8_t> bytes, std::string discriminator = {});
enum class ResourceDomain : unsigned
{
    primary = 0,
    secondary = 1,
    one = primary,
    two = secondary
};
using OriginalDisc = ResourceDomain;
uint64_t resource_hash(std::span<const uint8_t> bytes);
Result<std::vector<uint8_t>> read_bundle(const std::filesystem::path& root, unsigned kind, ResourceDomain domain, unsigned id = 0);
class ResourceBundle
{
    std::filesystem::path root_;

public:
    explicit ResourceBundle(const std::filesystem::path& root);
    Result<std::vector<uint8_t>> try_dat_entry(ResourceDomain domain, unsigned id) const;
    std::vector<uint8_t> dat_entry(ResourceDomain domain, unsigned id) const;
    Result<std::vector<uint8_t>> try_prg_image(ResourceDomain domain = ResourceDomain::primary) const;
    Result<std::vector<uint8_t>> try_prg_entry(ResourceDomain domain, unsigned id) const;
};
using OriginalDiscResources = ResourceBundle;
}    //namespace s2ne
