#pragma once
#include "original_save.hpp"
#include <memory>
namespace s2ne
{
// 412F1D ->4138F7: sell only carried, priced items; equipped items are not
// silently removed. On-disk inventory is the original shared army byte array.
class OriginalShop
{
    std::vector<uint8_t> items_;
    unsigned count_{ };
    s2ne::Result<unsigned> try_price(unsigned id) const
    {
        if (!id || id > count_)
        {
            return s2ne::Result<unsigned>::failure(make_error("Original shop item ID"));
        }
        size_t p = 2 + size_t(id - 1) * 38 + 0x1A;
        return s2ne::Result<unsigned>::success(items_.at(p) | (unsigned(items_.at(p + 1)) << 8));
    }
    unsigned price(unsigned id) const
    {
        auto result = try_price(id);
        return std::move(result).value();
    }
    static uint32_t get(std::span<const uint8_t> bytes, size_t p)
    {
        return uint32_t(bytes[p]) | (uint32_t(bytes[p + 1]) << 8) | (uint32_t(bytes[p + 2]) << 16) | (uint32_t(bytes[p + 3]) << 24);
    }

public:
    struct InitializationTag
    {
    };
    OriginalShop(const std::filesystem::path& root, InitializationTag) { }
    s2ne::Result<bool> initialize(const std::filesystem::path& root)
    {
        std::ifstream f(root / "ITEMSP.BIN", std::ios::binary);
        if (!f)
        {
            return s2ne::Result<bool>::failure(make_error("Missing original shop items"));
        }
        items_ = { std::istreambuf_iterator<char>(f), { } };
        if (items_.size() < 2)
        {
            return s2ne::Result<bool>::failure(make_error("Truncated shop item header"));
        }
        count_ = unsigned(items_[0]) | (unsigned(items_[1]) << 8);
        if (!count_ || count_ > 10000 || items_.size() < 2 + size_t(count_) * 38)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid shop item table"));
        }

        return s2ne::Result<bool>::success(true);
    }
    explicit OriginalShop(const std::filesystem::path& root)
    {
        auto result = initialize(root);
        if (!result)
        {
            fatal(result.error());
        }
    }
    static s2ne::Result<std::unique_ptr<OriginalShop>> create(const std::filesystem::path& root)
    {
        auto result = std::make_unique<OriginalShop>(root, InitializationTag{ });
        auto ready = result->initialize(root);
        if (!ready)
        {
            return s2ne::Result<std::unique_ptr<OriginalShop>>::failure(ready.error());
        }
        return s2ne::Result<std::unique_ptr<OriginalShop>>::success(std::move(result));
    }

    s2ne::Result<std::vector<unsigned>> try_sale_items(const OriginalSave::State& state) const
    {
        if (state.groups[0].inventory.size() != count_)
        {
            return s2ne::Result<std::vector<unsigned>>::failure(make_error("Original shop inventory framing"));
        }
        std::vector<unsigned> result;
        for (unsigned id = 1; id <= count_; ++id)
        {
            if (state.groups[0].inventory[id - 1] && price(id))
            {
                result.push_back(id);
            }
        }
        return s2ne::Result<std::vector<unsigned>>::success(result);
    }
    std::vector<unsigned> sale_items(const OriginalSave::State& state) const
    {
        auto result = try_sale_items(state);
        return std::move(result).value();
    }
    std::vector<uint8_t> name(unsigned id) const
    {
        (void)price(id);
        size_t p = 2 + size_t(id - 1) * 38 + 2, end = p;
        while (end < p + 12 && items_.at(end))
        {
            ++end;
        }
        return { items_.begin() + p, items_.begin() + end };
    }
    unsigned buy_price(const OriginalSave::State& state, unsigned id) const
    {
        return price(id) * ((get(state.groups[0].metadata, 4) & 0x10) ? 8u : 10u) / 10;
    }
    bool buy(OriginalSave::State& state, unsigned id, unsigned quantity) const
    {
        if (state.groups[0].inventory.size() != count_ || !id || id > count_ || !quantity)
        {
            return false;
        }
        uint64_t cost = uint64_t(quantity) * buy_price(state, id);
        auto& army = state.groups[0];
        uint32_t money = get(army.metadata, 0);
        if (cost > money)
        {
            return false;
        }
        money -= uint32_t(cost);
        for (unsigned i = 0; i < 4; ++i)
        {
            army.metadata[i] = uint8_t(money >> (i * 8));
        }
        army.inventory[id - 1] = uint8_t(std::min(99u, unsigned(army.inventory[id - 1]) + quantity));
        return true;
    }
    unsigned sale_price(const OriginalSave::State& state, unsigned id) const
    {
        //44926C returns army+DE, serialized at metadata+4.
        return price(id) * ((get(state.groups[0].metadata, 4) & 0x10) ? 7u : 5u) / 10;
    }
    bool sell(OriginalSave::State& state, unsigned id, unsigned quantity) const
    {
        if (state.groups[0].inventory.size() != count_ || !id || id > count_ || !quantity || quantity > state.groups[0].inventory[id - 1] || !price(id))
        {
            return false;
        }
        auto& army = state.groups[0];
        uint32_t money = get(army.metadata, 0) + uint32_t(quantity * sale_price(state, id));
        //413F16 commits currency first, then subtracts the original inventory byte.
        for (unsigned i = 0; i < 4; ++i)
        {
            army.metadata[i] = uint8_t(money >> (i * 8));
        }
        army.inventory[id - 1] = uint8_t(army.inventory[id - 1] - quantity);
        return true;
    }
};
}    //namespace s2ne
