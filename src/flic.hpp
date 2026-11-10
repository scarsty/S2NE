#pragma once
#include "original_disc.hpp"
#include "resource_result.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>
namespace s2ne
{
class Flic
{
    std::vector<uint8_t> data, pixels;
    std::array<uint8_t, 768> palette{ };
    size_t position = 128, first_position_ = 128, cursor{ }, limit{ };
    unsigned frame{ }, chunk_type_{ };
    std::optional<Error> error_;
    void reject(std::string message)
    {
        if (!error_)
        {
            error_ = make_error(std::move(message));
        }
    }
    uint8_t byte()
    {
        if (cursor >= limit)
        {
            reject("Truncated FLIC chunk frame=" + std::to_string(frame));
            return 0;
        }
        return data[cursor++];
    }
    unsigned word()
    {
        unsigned n = byte();
        return n | (unsigned(byte()) << 8);
    }
    void put(int x, int y, uint8_t value)
    {
        if (x < 0 || y < 0 || x >= width || y >= height)
        {
            reject("Invalid FLIC pixel position");
            return;
        }
        pixels[size_t(y) * width + x] = value;
    }

public:
    int width{ }, height{ };
    unsigned frames{ }, delay{ };
    Flic() = default;
    static Result<Flic> create(std::vector<uint8_t> bytes)
    {
        Flic result;
        result.data = std::move(bytes);
        ResourceReader reader(result.data);
        if (!reader.extent(0, 128))
        {
            return Result<Flic>::failure(reader.error());
        }
        if (reader.number(4) != 0xAF12)
        {
            return Result<Flic>::failure(make_error("Unsupported FLIC type"));
        }
        result.frames = reader.number(6);
        result.width = reader.number(8);
        result.height = reader.number(10);
        if (reader.number(12) != 8 || result.width != 640 || result.height < 1 || result.height > 480 || !result.frames)
        {
            return Result<Flic>::failure(make_error("Unsupported FLIC dimensions"));
        }
        result.delay = reader.number(16, 4);
        if (!result.delay || result.delay > 1000)
        {
            return Result<Flic>::failure(make_error("Invalid FLIC frame delay"));
        }
        size_t first = reader.number(80, 4);
        if (first >= 128 && first < result.data.size())
        {
            result.position = first;
        }
        result.first_position_ = result.position;
        result.pixels.resize(size_t(result.width) * result.height);
        return Result<Flic>::success(std::move(result));
    }
    static Result<Flic> try_from_disc(const std::filesystem::path& root, unsigned id, OriginalDisc disc = OriginalDisc::one)
    {
        if (id < 21 || id > 25)
        {
            return Result<Flic>::failure(make_error("Invalid original movie resource ID"));
        }
        auto loaded = OriginalDiscResources(root).try_dat_entry(disc, id);
        if (!loaded)
        {
            return Result<Flic>::failure(loaded.error());
        }
        auto bytes = std::move(loaded).value();
        ResourceReader reader(bytes);
        if (!reader.extent(0, 128))
        {
            return Result<Flic>::failure(reader.error());
        }
        auto put = [&](size_t p, unsigned value, unsigned count)
        {
            for (unsigned i = 0; i < count; ++i)
            {
                bytes[p + i] = uint8_t(value >> (i * 8));
            }
        };
        unsigned a = reader.number(0, 4), b = reader.number(80, 4);
        put(0, b, 4);
        put(80, a, 4);
        a = reader.number(6);
        b = reader.number(14);
        put(6, b, 2);
        put(14, a, 2);
        put(4, reader.number(4) - 0x923, 2);
        put(8, reader.number(8) - 56, 2);
        put(10, reader.number(10) - 11, 2);
        put(12, reader.number(12) - 8, 2);
        return create(std::move(bytes));
    }
    static Flic from_disc(const std::filesystem::path& root, unsigned id, OriginalDisc disc = OriginalDisc::one)
    {
        auto result = try_from_disc(root, id, disc);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    static std::vector<uint8_t> read(const std::filesystem::path& path)
    {
        auto result = read_resource(path);
        if (!result)
        {
            fatal(result.error());
        }
        return std::move(result).value();
    }
    explicit Flic(const std::filesystem::path& path) : Flic(read(path)) { }
    explicit Flic(std::vector<uint8_t> bytes)
    {
        auto result = create(std::move(bytes));
        if (!result)
        {
            fatal(result.error());
        }
        *this = std::move(result).value();
    }
    void rewind()
    {
        position = first_position_;
        frame = 0;
        std::fill(pixels.begin(), pixels.end(), 0);
        palette.fill(0);
    }
    Result<bool> try_next(std::vector<uint8_t>& rgba)
    {
        auto candidate = *this;
        std::vector<uint8_t> output;
        auto result = candidate.decode_next(output);
        if (!result)
        {
            return result;
        }
        if (result.value())
        {
            *this = std::move(candidate);
            rgba = std::move(output);
        }
        return result;
    }
    bool next(std::vector<uint8_t>& rgba)
    {
        auto result = try_next(rgba);
        if (!result)
        {
            fatal(result.error());
        }
        return result.value();
    }

private:
    Result<bool> decode_next(std::vector<uint8_t>& rgba)
    {
        if (frame >= frames)
        {
            return Result<bool>::success(false);
        }
        cursor = position;
        limit = data.size();
        size_t size = word();
        size |= size_t(word()) << 16;
        if (size < 16 || size > data.size() - position || word() != 0xF1FA)
        {
            return Result<bool>::failure(make_error("Invalid FLIC frame"));
        }
        unsigned chunks = word();
        size_t end = position + size;
        cursor = position + 16;
        for (unsigned c = 0; c < chunks; ++c)
        {
            size_t start = cursor;
            limit = end;
            size_t length = word();
            length |= size_t(word()) << 16;
            unsigned type = word();
            if (length < 6 || length > end - start)
            {
                return Result<bool>::failure(make_error("Invalid FLIC chunk length"));
            }
            limit = start + length;
            chunk_type_ = type;
            if (type == 4 || type == 11)
            {
                unsigned packets = word(), index = 0;
                for (unsigned p = 0; p < packets; ++p)
                {
                    index += byte();
                    unsigned count = byte();
                    if (!count)
                    {
                        count = 256;
                    }
                    if (index + count > 256)
                    {
                        return Result<bool>::failure(make_error("Invalid FLIC palette"));
                    }
                    for (unsigned i = 0; i < count * 3; ++i)
                    {
                        unsigned value = byte();
                        palette[index * 3 + i] = uint8_t(type == 11 ? value * 255 / 63 : value);
                    }
                    index += count;
                }
            }
            else if (type == 15)
            {
                for (int y = 0; y < height; ++y)
                {
                    byte();
                    int x = 0;
                    while (x < width && !error_)
                    {
                        int n = int8_t(byte());
                        if (!n)
                        {
                            return Result<bool>::failure(make_error("Invalid empty FLIC BRUN run"));
                        }
                        if (n >= 0)
                        {
                            auto value = byte();
                            while (n--)
                            {
                                put(x++, y, value);
                            }
                        }
                        else
                        {
                            while (n++)
                            {
                                put(x++, y, byte());
                            }
                        }
                    }
                    if (x != width)
                    {
                        return Result<bool>::failure(make_error("Incomplete FLIC BRUN row"));
                    }
                }
            }
            else if (type == 7)
            {
                unsigned lines = word();
                int y = 0;
                while (lines && !error_)
                {
                    unsigned control = word();
                    if ((control & 0xC000) == 0xC000)
                    {
                        y -= int16_t(control);
                        continue;
                    }
                    if (control & 0x8000)
                    {
                        put(width - 1, y, uint8_t(control));
                        continue;
                    }
                    int x = 0;
                    for (unsigned p = 0; p < control; ++p)
                    {
                        x += byte();
                        int n = int8_t(byte());
                        if (n >= 0)
                        {
                            while (n--)
                            {
                                put(x++, y, byte());
                                put(x++, y, byte());
                            }
                        }
                        else
                        {
                            uint8_t a = byte(), b = byte();
                            while (n++)
                            {
                                put(x++, y, a);
                                put(x++, y, b);
                            }
                        }
                    }
                    ++y;
                    --lines;
                }
            }
            else if (type == 16)
            {
                if (length != 6 + size_t(width) * height && length != 4 + size_t(width) * height)
                {
                    return Result<bool>::failure(make_error("Invalid native FLIC COPY size"));
                }
                limit = std::min(data.size(), cursor + size_t(width) * height);
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        put(x, y, byte());
                    }
                }
            }
            else if (type == 13) { std::fill(pixels.begin(), pixels.end(), 0); }
            else if (type != 18)
            {
                return Result<bool>::failure(make_error("Unsupported FLIC chunk: " + std::to_string(type)));
            }
            cursor = start + length;
        }
        if (error_)
        {
            return Result<bool>::failure(*error_);
        }
        position = end;
        ++frame;
        rgba.assign(640 * 480 * 4, 0);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                size_t dst = (size_t(y + (480 - height) / 2) * 640 + x) * 4;
                unsigned color = pixels[size_t(y) * width + x];
                for (int c = 0; c < 3; ++c)
                {
                    rgba[dst + c] = palette[color * 3 + c];
                }
                rgba[dst + 3] = 255;
            }
        }
        return Result<bool>::success(true);
    }
};
}    //namespace s2ne
