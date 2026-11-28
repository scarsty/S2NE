#pragma once
#include "save.h"
#include <memory>
namespace s2ne
{
// 412F1D ->4138F7: sell only carried, priced items; equipped items are not
// silently removed. On-disk inventory is the original shared army byte array.
class Shop
{
    std::vector<uint8_t> items_;
    unsigned count_{};
    s2ne::Result<unsigned> try_price(unsigned id) const;
    unsigned price(unsigned id) const;
    static uint32_t get(std::span<const uint8_t> bytes, size_t p);

public:
    struct InitializationTag
    {
    };
    Shop(const std::filesystem::path& root, InitializationTag);
    s2ne::Result<bool> initialize(const std::filesystem::path& root);
    explicit Shop(const std::filesystem::path& root);
    static s2ne::Result<std::unique_ptr<Shop>> create(const std::filesystem::path& root);

    s2ne::Result<std::vector<unsigned>> try_sale_items(const Save::State& state) const;
    std::vector<unsigned> sale_items(const Save::State& state) const;
    std::vector<uint8_t> name(unsigned id) const;
    unsigned buy_price(const Save::State& state, unsigned id) const;
    bool buy(Save::State& state, unsigned id, unsigned quantity) const;
    unsigned sale_price(const Save::State& state, unsigned id) const;
    bool sell(Save::State& state, unsigned id, unsigned quantity) const;
};
}    //namespace s2ne
