#include "xmi.h"

namespace s2ne
{
auto original_xmi_midi(std::span<const uint8_t> xmi) -> Result<std::vector<uint8_t>>
{
    struct Event
    {
        uint32_t tick;
        std::vector<uint8_t> data;
    };
    auto failure = []
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid original XMI events"));
    };
    size_t start = 0, length = 0;
    for (size_t i = 0; i + 8 <= xmi.size(); ++i)
    {
        if (std::equal(xmi.begin() + i, xmi.begin() + i + 4, "EVNT"))
        {
            start = i + 8;
            for (unsigned j = 0; j < 4; ++j) { length = (length << 8) | xmi[i + 4 + j]; }
            break;
        }
    }
    if (!start || length > xmi.size() - start) { return failure(); }
    auto bytes = xmi.subspan(start, length);
    std::vector<Event> events;
    uint32_t tick = 0;
    size_t p = 0;
    auto vlq = [&](uint32_t& value)
    {
        value = 0;
        for (unsigned i = 0; i < 4 && p < bytes.size(); ++i)
        {
            unsigned b = bytes[p++];
            value = (value << 7) | (b & 127);
            if (!(b & 128)) { return true; }
        }
        return false;
    };
    while (p < bytes.size())
    {
        while (p < bytes.size() && bytes[p] < 128)
        {
            if (bytes[p] > UINT32_MAX - tick) { return failure(); }
            tick += bytes[p++];
        }
        if (p == bytes.size()) { return failure(); }
        unsigned status = bytes[p++], type = status & 0xf0;
        if (status == 0xff || status == 0xf0 || status == 0xf7)
        {
            unsigned meta = 0;
            if (status == 0xff)
            {
                if (p == bytes.size()) { return failure(); }
                meta = bytes[p++];
            }
            uint32_t n = 0;
            if (!vlq(n) || n > bytes.size() - p) { return failure(); }
            if (status == 0xff && meta == 0x2f) { break; }
            // tempo不改变XMI的120Hz；其它元信息与SysEx保留原数据。
            if (!(status == 0xff && meta == 0x51))
            {
                std::vector<uint8_t> data{ uint8_t(status) };
                if (status == 0xff) { data.push_back(uint8_t(meta)); }
                std::vector<uint8_t> size{ uint8_t(n & 127) };
                for (auto v = n >> 7; v; v >>= 7) { size.insert(size.begin(), uint8_t((v & 127) | 128)); }
                data.insert(data.end(), size.begin(), size.end());
                data.insert(data.end(), bytes.begin() + p, bytes.begin() + p + n);
                events.push_back({ tick, std::move(data) });
            }
            p += n;
            continue;
        }
        if (type < 0x80 || type > 0xe0) { return failure(); }
        unsigned n = (type == 0xc0 || type == 0xd0) ? 1 : 2;
        if (n > bytes.size() - p) { return failure(); }
        for (unsigned i = 0; i < n; ++i)
        {
            if (bytes[p + i] & 128) { return failure(); }
        }
        std::vector<uint8_t> data{ uint8_t(status) };
        data.insert(data.end(), bytes.begin() + p, bytes.begin() + p + n);
        p += n;
        events.push_back({ tick, data });
        if (type == 0x90)
        {
            uint32_t duration = 0;
            if (!vlq(duration) || duration > UINT32_MAX - tick) { return failure(); }
            events.push_back({ tick + duration, { uint8_t(0x80 | (status & 15)), data[1], 0 } });
        }
    }
    std::stable_sort(events.begin(), events.end(), [](const auto& a, const auto& b)
        {
            return a.tick < b.tick;
        });
    std::vector<uint8_t> track{ 0, 0xff, 0x51, 3, 0x07, 0xa1, 0x20 };
    uint32_t previous = 0;
    for (const auto& event : events)
    {
        uint32_t delta = event.tick - previous;
        previous = event.tick;
        std::vector<uint8_t> time{ uint8_t(delta & 127) };
        for (delta >>= 7; delta; delta >>= 7) { time.insert(time.begin(), uint8_t((delta & 127) | 128)); }
        track.insert(track.end(), time.begin(), time.end());
        track.insert(track.end(), event.data.begin(), event.data.end());
    }
    uint32_t tail = tick > previous ? tick - previous : 0;
    std::vector<uint8_t> endTime{ uint8_t(tail & 127) };
    for (tail >>= 7; tail; tail >>= 7) { endTime.insert(endTime.begin(), uint8_t((tail & 127) | 128)); }
    track.insert(track.end(), endTime.begin(), endTime.end());
    track.insert(track.end(), { 0xff, 0x2f, 0 });
    std::vector<uint8_t> midi{ 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0, 60, 'M', 'T', 'r', 'k' };
    for (int shift = 24; shift >= 0; shift -= 8) { midi.push_back(uint8_t(track.size() >> shift)); }
    midi.insert(midi.end(), track.begin(), track.end());
    return Result<std::vector<uint8_t>>::success(std::move(midi));
}

auto original_xmi_resources(const std::filesystem::path& root) -> Result<std::vector<std::vector<uint8_t>>>
{
    using Resources = std::vector<std::vector<uint8_t>>;
    auto pal = read_shared_resource(root / "SUPERSP.PAL"), bin = read_shared_resource(root / "SUPERSP.BIN");
    if (!pal) { return Result<Resources>::failure(pal.error()); }
    if (!bin) { return Result<Resources>::failure(bin.error()); }
    ResourceReader p(pal.value());
    auto offset = p.number(984, 4), length = p.number(988, 4);
    ResourceReader b(bin.value());
    if (offset < 28 || length < 28 || !b.extent(offset - 28, length - 28)) { return Result<Resources>::failure(make_error("Invalid midisp index")); }
    Resources resources;
    size_t base = offset - 28;
    if (b.number(base) != 37 || !b.extent(base + 2, 37 * 8)) { return Result<Resources>::failure(make_error("Invalid midisp count")); }
    for (unsigned id = 1; id <= 37; ++id)
    {
        size_t entry = base + 2 + (id - 1) * 8;
        auto start = b.number(entry, 4), size = b.number(entry + 4, 4);
        if (start > length - 28 || size > length - 28 - start || !b.extent(base + start, size)) { return Result<Resources>::failure(make_error("Invalid midisp payload")); }
        resources.emplace_back(bin.value().begin() + base + start, bin.value().begin() + base + start + size);
    }
    return Result<Resources>::success(std::move(resources));
}
}    //namespace s2ne
