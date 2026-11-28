#pragma once
#include "disc.h"
#include "resource_result.h"
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
    std::array<uint8_t, 768> palette{};
    size_t position = 128, first_position_ = 128, cursor{}, limit{};
    unsigned frame{}, chunk_type_{};
    std::optional<Error> error_;
    void reject(std::string message);
    uint8_t byte();
    unsigned word();
    void put(int x, int y, uint8_t value);

public:
    int width{}, height{};
    unsigned frames{}, delay{};
    Flic() = default;
    static Result<Flic> create(std::vector<uint8_t> bytes);
    static Result<Flic> try_from_disc(const std::filesystem::path& root, unsigned id, Disc disc = Disc::one);
    static Flic from_disc(const std::filesystem::path& root, unsigned id, Disc disc = Disc::one);
    static std::vector<uint8_t> read(const std::filesystem::path& path);
    explicit Flic(const std::filesystem::path& path);
    explicit Flic(std::vector<uint8_t> bytes);
    void rewind();
    Result<bool> try_next(std::vector<uint8_t>& rgba);
    bool next(std::vector<uint8_t>& rgba);

private:
    Result<bool> decode_next(std::vector<uint8_t>& rgba);
};
}    //namespace s2ne
