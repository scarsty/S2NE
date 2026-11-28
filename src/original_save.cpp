#include "original_save.h"

namespace s2ne
{
auto OriginalSave::Group::role_selectors() const -> std::vector<int>
{
    std::vector<int> roles;
    roles.reserve(units.size());
    for (const auto& record : units)
    {
        roles.push_back(int(int16_t(unsigned(record[8]) | (unsigned(record[9]) << 8))));
    }
    return roles;
}

auto OriginalSave::State::battle_context() const -> bool { return u32(globals_4f7cd0, 4) == 0; }

auto OriginalSave::u32(std::span<const uint8_t> b, size_t p) -> uint32_t
{
    return uint32_t(b[p]) | (uint32_t(b[p + 1]) << 8) | (uint32_t(b[p + 2]) << 16) | (uint32_t(b[p + 3]) << 24);
}

[[noreturn]] auto OriginalSave::fail() -> void { fatal("Invalid or unsupported original S2NE save"); }

auto OriginalSave::check_items(size_t count) -> void
{
    if (count > 32767)
    {
        fail();
    }
}

OriginalSave::Reader::Reader(std::span<const uint8_t> bytes) : bytes_(bytes) {}

auto OriginalSave::Reader::take(std::span<uint8_t> dest) -> void
{
    if (dest.size() > bytes_.size() - position_)
    {
        fail();
    }
    std::copy_n(bytes_.begin() + position_, dest.size(), dest.begin());
    position_ += dest.size();
}

auto OriginalSave::Reader::dword() -> uint32_t
{
    std::array<uint8_t, 4> b{};
    take(b);
    return u32(b, 0);
}

auto OriginalSave::Reader::word() -> uint16_t
{
    std::array<uint8_t, 2> b{};
    take(b);
    return uint16_t(b[0] | (unsigned(b[1]) << 8));
}

auto OriginalSave::Reader::remaining() const -> size_t { return bytes_.size() - position_; }

auto OriginalSave::append(std::vector<uint8_t>& out, std::span<const uint8_t> b) -> void { out.insert(out.end(), b.begin(), b.end()); }

auto OriginalSave::number(std::vector<uint8_t>& out, uint32_t value, unsigned width) -> void
{
    for (unsigned i = 0; i < width; ++i)
    {
        out.push_back(uint8_t(value >> (i * 8)));
    }
}

auto OriginalSave::validate(const State& s, size_t items) -> void
{
    check_items(items);
    for (const auto& g : s.groups)
    {
        if (g.units.size() > maximum_units || g.inventory.size() != items)
        {
            fail();
        }
    }
    if (s.event_states.size() > 32767 || (!s.battle_context() && !s.event_states.empty()))
    {
        fail();
    }
}

auto OriginalSave::item_count(std::span<const uint8_t> itemsp) -> size_t
{
    if (itemsp.size() < 2)
    {
        fail();
    }
    size_t count = unsigned(itemsp[0]) | (unsigned(itemsp[1]) << 8);
    check_items(count);
    if (itemsp.size() < 2 + count * 38)
    {
        fail();
    }
    return count;
}

auto OriginalSave::try_decode(std::span<const uint8_t> bytes, size_t items) -> Result<State>
{
    auto invalid = []
    {
        return Result<State>::failure(make_error("Invalid or unsupported original S2NE save"));
    };
    if (items > 32767 || bytes.size() < fixed_size || !std::equal(signature.begin(), signature.end(), bytes.begin()))
    {
        return invalid();
    }
    size_t cursor = fixed_size;
    for (unsigned group = 0; group < 2; ++group)
    {
        if (cursor > bytes.size() || bytes.size() - cursor < 4)
        {
            return invalid();
        }
        size_t count = u32(bytes, cursor);
        cursor += 4;
        if (count > maximum_units)
        {
            return invalid();
        }
        size_t size = count * unit_size + 18 + items + 33;
        if (size > bytes.size() - cursor)
        {
            return invalid();
        }
        cursor += size;
    }
    if (u32(bytes, 429) == 0)
    {
        if (bytes.size() - cursor < 2)
        {
            return invalid();
        }
        size_t count = bytes[cursor] | (unsigned(bytes[cursor + 1]) << 8);
        cursor += 2;
        if (count > 32767 || count * 2 != bytes.size() - cursor)
        {
            return invalid();
        }
        cursor += count * 2;
    }
    if (cursor != bytes.size())
    {
        return invalid();
    }
    return Result<State>::success(decode_validated(bytes, items));
}

auto OriginalSave::decode(std::span<const uint8_t> bytes, size_t items) -> State
{
    auto result = try_decode(bytes, items);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto OriginalSave::decode_validated(std::span<const uint8_t> bytes, size_t items) -> State
{
    check_items(items);
    Reader r(bytes);
    State s;
    std::array<uint8_t, 9> magic{};
    r.take(magic);
    if (magic != signature)
    {
        fail();
    }
    r.take(s.description);
    r.take(s.attribution);
    r.take(s.flags_4f7d10);
    r.take(s.settings_4f7cf0);
    r.take(s.globals_4f7cd0);
    r.take(s.value_4f705c);
    r.take(s.words_4f6fbc);
    r.take(s.stage_4fb478);
    for (auto& g : s.groups)
    {
        auto count = r.dword();
        if (count > maximum_units)
        {
            fail();
        }
        if (size_t(count) * unit_size + 18 + items + 33 > r.remaining())
        {
            fail();
        }
        g.units.resize(count);
        for (auto& unit : g.units)
        {
            r.take(unit);
        }
        r.take(g.metadata);
        g.inventory.resize(items);
        r.take(g.inventory);
        r.take(g.status);
    }
    if (s.battle_context())
    {
        unsigned count = r.word();
        if (count > 32767 || size_t(count) * 2 != r.remaining())
        {
            fail();
        }
        s.event_states.resize(count);
        for (auto& state : s.event_states)
        {
            state = r.word();
        }
    }
    if (r.remaining())
    {
        fail();
    }
    validate(s, items);
    return s;
}

auto OriginalSave::try_encode(const State& s, size_t items) -> Result<std::vector<uint8_t>>
{
    if (items > 32767 || s.event_states.size() > 32767 || (!s.battle_context() && !s.event_states.empty()))
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid original save state"));
    }
    for (const auto& group : s.groups)
    {
        if (group.units.size() > maximum_units || group.inventory.size() != items)
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid original save army framing"));
        }
    }
    return Result<std::vector<uint8_t>>::success(encode_validated(s, items));
}

auto OriginalSave::encode(const State& s, size_t items) -> std::vector<uint8_t>
{
    auto result = try_encode(s, items);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto OriginalSave::encode_validated(const State& s, size_t items) -> std::vector<uint8_t>
{
    validate(s, items);
    std::vector<uint8_t> out;
    append(out, signature);
    append(out, s.description);
    append(out, s.attribution);
    append(out, s.flags_4f7d10);
    append(out, s.settings_4f7cf0);
    append(out, s.globals_4f7cd0);
    append(out, s.value_4f705c);
    append(out, s.words_4f6fbc);
    append(out, s.stage_4fb478);
    for (const auto& g : s.groups)
    {
        number(out, uint32_t(g.units.size()), 4);
        for (const auto& unit : g.units)
        {
            append(out, unit);
        }
        append(out, g.metadata);
        append(out, g.inventory);
        append(out, g.status);
    }
    if (s.battle_context())
    {
        number(out, uint32_t(s.event_states.size()), 2);
        for (auto state : s.event_states)
        {
            number(out, state, 2);
        }
    }
    return out;
}

auto OriginalSave::try_load(const std::filesystem::path& path, size_t items) -> Result<State>
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return Result<State>::failure(make_error("Cannot open original save"));
    }
    std::vector<uint8_t> bytes{ std::istreambuf_iterator<char>(file), {} };
    if (file.bad())
    {
        return Result<State>::failure(make_error("Cannot read original save"));
    }
    return try_decode(bytes, items);
}

auto OriginalSave::load(const std::filesystem::path& path, size_t items) -> State
{
    auto result = try_load(path, items);
    if (!result)
    {
        fatal(result.error());
    }
    return std::move(result).value();
}

auto OriginalSave::slot_path(const std::filesystem::path& root, unsigned slot) -> std::filesystem::path
{
    if (slot < 1 || slot > 53)
    {
        fail();
    }
    return root / "SAVE" / ("UJ" + std::string(slot < 10 ? "0" : "") + std::to_string(slot) + ".SAV");
}
}    //namespace s2ne
