#pragma once
#include "runtime_error.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>
namespace s2ne
{
enum class ResourceDomain : unsigned
{
    primary = 0,
    secondary = 1,
    one = primary,
    two = secondary
};
using OriginalDisc = ResourceDomain;
inline uint64_t resource_hash(std::span<const uint8_t> bytes)
{
    uint64_t hash = 14695981039346656037ull;
    for (auto byte : bytes)
    {
        hash = (hash ^ byte) * 1099511628211ull;
    }
    return hash;
}
inline Result<std::vector<uint8_t>> read_bundle(const std::filesystem::path& root, unsigned kind, ResourceDomain domain, unsigned id = 0)
{
    auto fail = [](const char* message)
    {
        return Result<std::vector<uint8_t>>::failure(make_error(message));
    };
    if (unsigned(domain) > 1 || (kind != 1 && kind != 2))
    {
        return fail("Invalid logical resource domain/type");
    }
    std::ifstream stream(root / (kind == 1 ? "SUPERSP.DAT" : "SUPERSP.PRG"), std::ios::binary);
    std::ifstream index(root / "SUPERSP.PAL", std::ios::binary);
    if (!stream || !index)
    {
        return fail("Missing merged resource bundle/index");
    }
    std::array<uint8_t, 40> header{ };
    if (!stream.read(reinterpret_cast<char*>(header.data()), header.size()))
    {
        return fail("Truncated resource bundle header");
    }
    auto number = [](std::span<const uint8_t> bytes, size_t at, unsigned count = 4)
    {
        uint64_t value = 0;
        for (unsigned n = 0; n < count; ++n)
        {
            value |= uint64_t(bytes[at + n]) << (n * 8);
        }
        return value;
    };
    if (!std::equal(header.begin(), header.begin() + 8, "S2NEPK03") || number(header, 8) != 3 || number(header, 12) != kind || number(header, 20) != 2)
    {
        return fail("Invalid merged resource magic/version/type");
    }
    std::vector<uint8_t> pal{ std::istreambuf_iterator<char>(index), { } };
    if (pal.size() < 1392 || number(header, 16) != pal.size())
    {
        return fail("Merged resource PAL size mismatch");
    }
    std::vector<uint8_t> embedded(pal.size());
    if (!stream.read(reinterpret_cast<char*>(embedded.data()), embedded.size()) || embedded != pal)
    {
        return fail("Merged resource PAL mismatch");
    }
    constexpr std::array<unsigned, 5> biases{ 56, 11, 8, 10, 20 };
    struct Mapping
    {
        unsigned domain, id;
        uint64_t offset, size;
    };
    std::vector<Mapping> expected;
    for (unsigned d = 0; d < 2; ++d)
    {
        if (kind == 1)
        {
            size_t table = 808;
            for (unsigned group = 0; group < 5; ++group)
            {
                auto encoded = number(pal, 768 + group * 4);
                if (encoded < biases[group])
                {
                    return fail("Invalid PAL group count");
                }
                uint64_t count = encoded - biases[group];
                if (table > pal.size() || count > (pal.size() - table) / 8)
                {
                    return fail("Invalid PAL table extent");
                }
                if (group == (d ? 4 : 2))
                {
                    for (unsigned i = 0; i < count; ++i)
                    {
                        auto offset = number(pal, table + i * 8), size = number(pal, table + i * 8 + 4), bias = uint64_t(biases[group] + i);
                        if (offset < bias || size < bias)
                        {
                            return fail("Invalid PAL resource span");
                        }
                        expected.push_back({ d, i + 1, offset - bias, size - bias });
                    }
                }
                table += size_t(count) * 8;
            }
        }
        else
        {
            for (unsigned i = 0; i < 9; ++i)
            {
                auto offset = number(pal, 1320 + i * 8), size = number(pal, 1324 + i * 8), bias = uint64_t(10 + i);
                if (offset < bias || size < bias)
                {
                    return fail("Invalid PAL PRG span");
                }
                expected.push_back({ d, i + 1, offset - bias, size - bias });
            }
        }
    }
    if (number(header, 24) != expected.size())
    {
        return fail("Merged resource mapping count mismatch");
    }
    uint64_t extentCount = number(header, 28);
    if (extentCount == 0 || extentCount > 2 * (expected.size() * 2 + 1))
    {
        return fail("Invalid shared resource extent count");
    }
    uint64_t payload = number(header, 32, 8), base = 40 + pal.size() + 64 + (expected.size() + extentCount) * 32;
    stream.seekg(0, std::ios::end);
    auto length = stream.tellg();
    if (length < 0 || base > uint64_t(length) || payload != uint64_t(length) - base)
    {
        return fail("Merged resource payload length mismatch");
    }
    stream.seekg(40 + pal.size());
    std::array<uint64_t, 2> starts{ }, sizes{ }, hashes{ }, counts{ };
    for (unsigned d = 0; d < 2; ++d)
    {
        std::array<uint8_t, 32> descriptor{ };
        if (!stream.read(reinterpret_cast<char*>(descriptor.data()), 32))
        {
            return fail("Truncated source descriptor");
        }
        starts[d] = number(descriptor, 8, 8);
        sizes[d] = number(descriptor, 16, 8);
        hashes[d] = number(descriptor, 24, 8);
        counts[d] = number(descriptor, 4);
        if (number(descriptor, 0) != d || counts[d] == 0 || starts[d] != (d ? counts[0] : 0) || starts[d] > extentCount || counts[d] > extentCount - starts[d] || sizes[d] < 28 || sizes[d] > 1024ull * 1024 * 1024)
        {
            return fail("Invalid source descriptor");
        }
    }
    if (counts[0] + counts[1] != extentCount)
    {
        return fail("Unindexed resource extents");
    }
    if (kind == 2)
    {
        auto& tail = expected.back();
        if (tail.offset > sizes[1])
        {
            return fail("Invalid secondary PRG tail");
        }
        tail.size = sizes[1] - tail.offset;
    }
    uint64_t selectedOffset = 0, selectedSize = 0, selectedHash = 0;
    bool found = id == 0;
    std::array<uint64_t, 2> ends{ };
    std::vector<uint64_t> entryHashes;
    for (const auto& mapping : expected)
    {
        std::array<uint8_t, 32> record{ };
        if (!stream.read(reinterpret_cast<char*>(record.data()), 32))
        {
            return fail("Truncated merged resource index");
        }
        auto offset = number(record, 8, 8), size = number(record, 16, 8);
        if (number(record, 0) != mapping.domain || number(record, 4) != mapping.id || offset != mapping.offset || size != mapping.size || offset > sizes[mapping.domain] || size > sizes[mapping.domain] - offset)
        {
            return fail("Merged resource mapping/PAL mismatch");
        }
        entryHashes.push_back(number(record, 24, 8));
        ends[mapping.domain] = std::max(ends[mapping.domain], offset + size);
        if (mapping.domain == unsigned(domain) && mapping.id == id)
        {
            found = true;
            selectedOffset = offset;
            selectedSize = size;
            selectedHash = number(record, 24, 8);
        }
    }
    if (!found || ends != sizes)
    {
        return fail("Missing resource ID/unindexed source tail");
    }
    struct Extent
    {
        uint64_t logical, size, physical, hash;
    };
    std::vector<Extent> extents;
    std::vector<std::pair<uint64_t, uint64_t>> coverage;
    for (unsigned group = 0; group < 2; ++group)
    {
        uint64_t logicalEnd = 0;
        for (uint64_t i = 0; i < counts[group]; ++i)
        {
            std::array<uint8_t, 32> record{ };
            if (!stream.read(reinterpret_cast<char*>(record.data()), record.size()))
            {
                return fail("Truncated shared resource extent");
            }
            Extent extent{ number(record, 0, 8), number(record, 8, 8), number(record, 16, 8), number(record, 24, 8) };
            if (extent.logical != logicalEnd || extent.size == 0 || extent.size > sizes[group] - logicalEnd || extent.physical > payload || extent.size > payload - extent.physical)
            {
                return fail("Invalid shared resource extent");
            }
            logicalEnd += extent.size;
            coverage.emplace_back(extent.physical, extent.physical + extent.size);
            extents.push_back(extent);
        }
        if (logicalEnd != sizes[group])
        {
            return fail("Incomplete logical resource image");
        }
    }
    std::sort(coverage.begin(), coverage.end());
    uint64_t physicalEnd = 0;
    for (auto [start, end] : coverage)
    {
        if (start > physicalEnd)
        {
            return fail("Unindexed shared payload gap");
        }
        physicalEnd = std::max(physicalEnd, end);
    }
    if (physicalEnd != payload)
    {
        return fail("Unindexed shared payload tail");
    }
    unsigned d = unsigned(domain);
    if (id != 0)
    {
        std::vector<uint8_t> bytes(size_t(selectedSize), uint8_t{ });
        for (uint64_t i = starts[d]; i < starts[d] + counts[d]; ++i)
        {
            const auto& extent = extents[size_t(i)];
            auto begin = std::max(selectedOffset, extent.logical);
            auto end = std::min(selectedOffset + selectedSize, extent.logical + extent.size);
            if (begin >= end)
            {
                continue;
            }
            stream.seekg(base + extent.physical + begin - extent.logical);
            if (!stream.read(reinterpret_cast<char*>(bytes.data() + size_t(begin - selectedOffset)), std::streamsize(end - begin)))
            {
                return fail("Truncated shared entry bytes");
            }
        }
        if (resource_hash(bytes) != selectedHash)
        {
            return fail("Merged entry checksum mismatch");
        }
        return Result<std::vector<uint8_t>>::success(std::move(bytes));
    }
    std::vector<uint8_t> source(size_t(sizes[d]), uint8_t{ });
    for (uint64_t i = starts[d]; i < starts[d] + counts[d]; ++i)
    {
        const auto& extent = extents[size_t(i)];
        auto bytes = std::span<uint8_t>(source).subspan(size_t(extent.logical), size_t(extent.size));
        stream.seekg(base + extent.physical);
        if (!stream.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(bytes.size())) || resource_hash(bytes) != extent.hash)
        {
            return fail("Shared resource block checksum mismatch");
        }
    }
    if (resource_hash(source) != hashes[d])
    {
        return fail("Logical resource image checksum mismatch");
    }
    auto native = std::span<const uint8_t>(source);
    if (native.size() < 28 || number(native, 0) + 28 != native.size() || !std::equal(native.begin() + 8, native.begin() + 16, "SuperSp_"))
    {
        return fail("Merged native source header mismatch");
    }
    for (size_t n = 0; n < expected.size(); ++n)
    {
        const auto& mapping = expected[n];
        if (mapping.domain == d && resource_hash(native.subspan(size_t(mapping.offset), size_t(mapping.size))) != entryHashes[n])
        {
            return fail("Merged index entry checksum mismatch");
        }
    }
    return Result<std::vector<uint8_t>>::success(std::move(source));
}
class ResourceBundle
{
    std::filesystem::path root_;

public:
    explicit ResourceBundle(const std::filesystem::path& root) : root_(root) { }
    Result<std::vector<uint8_t>> try_dat_entry(ResourceDomain domain, unsigned id) const
    {
        if (!id)
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid DAT resource ID"));
        }
        return read_bundle(root_, 1, domain, id);
    }
    std::vector<uint8_t> dat_entry(ResourceDomain domain, unsigned id) const
    {
        auto result = try_dat_entry(domain, id);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    Result<std::vector<uint8_t>> try_prg_image(ResourceDomain domain = ResourceDomain::primary) const { return read_bundle(root_, 2, domain); }
    Result<std::vector<uint8_t>> try_prg_entry(ResourceDomain domain, unsigned id) const
    {
        if (!id)
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid PRG resource ID"));
        }
        return read_bundle(root_, 2, domain, id);
    }
};
using OriginalDiscResources = ResourceBundle;
}    //namespace s2ne
