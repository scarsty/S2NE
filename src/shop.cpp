#include "shop.h"

namespace s2ne
{
auto Shop::try_price(unsigned id) const -> s2ne::Result<unsigned>
{
    if (!id || id > count_)
    {
        return s2ne::Result<unsigned>::failure(make_error("Original shop item ID"));
    }
    size_t p = 2 + size_t(id - 1) * 38 + 0x1A;
    return s2ne::Result<unsigned>::success(items_.at(p) | (unsigned(items_.at(p + 1)) << 8));
}

auto Shop::price(unsigned id) const -> unsigned
{
    auto result = try_price(id);
    return std::move(result).value();
}

auto Shop::get(std::span<const uint8_t> bytes, size_t p) -> uint32_t
{
    return uint32_t(bytes[p]) | (uint32_t(bytes[p + 1]) << 8) | (uint32_t(bytes[p + 2]) << 16) | (uint32_t(bytes[p + 3]) << 24);
}

Shop::Shop(const std::filesystem::path& root, InitializationTag) {}

auto Shop::initialize(const std::filesystem::path& root) -> s2ne::Result<bool>
{
    std::ifstream f(root / "ITEMSP.BIN", std::ios::binary);
    if (!f)
    {
        return s2ne::Result<bool>::failure(make_error("Missing original shop items"));
    }
    items_ = { std::istreambuf_iterator<char>(f), {} };
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

Shop::Shop(const std::filesystem::path& root)
{
    auto result = initialize(root);
    if (!result)
    {
        fatal(result.error());
    }
}

auto Shop::create(const std::filesystem::path& root) -> s2ne::Result<std::unique_ptr<Shop>>
{
    auto result = std::make_unique<Shop>(root, InitializationTag{});
    auto ready = result->initialize(root);
    if (!ready)
    {
        return s2ne::Result<std::unique_ptr<Shop>>::failure(ready.error());
    }
    return s2ne::Result<std::unique_ptr<Shop>>::success(std::move(result));
}

auto Shop::try_sale_items(const Save::State& state) const -> s2ne::Result<std::vector<unsigned>>
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

auto Shop::sale_items(const Save::State& state) const -> std::vector<unsigned>
{
    auto result = try_sale_items(state);
    return std::move(result).value();
}

auto Shop::name(unsigned id) const -> std::vector<uint8_t>
{
    (void)price(id);
    size_t p = 2 + size_t(id - 1) * 38 + 2, end = p;
    while (end < p + 12 && items_.at(end))
    {
        ++end;
    }
    return { items_.begin() + p, items_.begin() + end };
}

auto Shop::buy_price(const Save::State& state, unsigned id) const -> unsigned
{
    return price(id) * ((get(state.groups[0].metadata, 4) & 0x10) ? 8u : 10u) / 10;
}

auto Shop::buy(Save::State& state, unsigned id, unsigned quantity) const -> bool
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

auto Shop::sale_price(const Save::State& state, unsigned id) const -> unsigned
{
    //44926C returns army+DE, serialized at metadata+4.
    return price(id) * ((get(state.groups[0].metadata, 4) & 0x10) ? 7u : 5u) / 10;
}

auto Shop::sell(Save::State& state, unsigned id, unsigned quantity) const -> bool
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
}    //namespace s2ne
