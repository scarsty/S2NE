#pragma once
#include "resource_result.h"
#include "simple_cc.h"
#include "texture_resources.h"
#include "title_image.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>
namespace s2ne
{
class Menu
{
    SharedBytes bin, pal;
    std::array<uint8_t, 768> title_palette_{};
    size_t strings{}, index{}, glyphs{};
    unsigned count{};
    std::vector<std::vector<uint8_t>> texts_;
    unsigned word(size_t p) const;
    unsigned dword(size_t p) const;
    size_t entry(unsigned id) const;

public:
    bool small_font = false;
    bool simplified_chinese = false;
    int selected = 0;
    int activated = -1;
    static constexpr std::array<unsigned, 5> ids{ 212, 213, 214, 123, 131 };
    static constexpr std::array<unsigned, 5> targets{ 0x40E53C, 0x4371E6, 0x40EBF3, 0x434103, 0x43183B };
    Menu() = default;
    static Result<Menu> create(const std::filesystem::path& root);
    explicit Menu(const std::filesystem::path& root);
    void move(int delta);
    int hit(float x, float y) const;
    bool mouse(float x, float y, bool click);
    unsigned activate();
    Result<std::vector<uint8_t>> try_text_bytes(unsigned id) const;
    const std::vector<uint8_t>& text_bytes(unsigned id) const;
    void draw_text(std::vector<uint8_t>& image, unsigned id, int x, int y, unsigned color) const;
    void set_simplified_chinese(bool enabled, const std::filesystem::path& dictionary_directory);
    // sub_422402: BIN 12 contains 16x16 Big5 glyphs; BIN 15 contains 8x15 ASCII.
    s2ne::Result<bool> try_draw_small_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false, std::vector<uint8_t>* indexed = nullptr) const;
    s2ne::Result<bool> try_draw_small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false, std::vector<uint8_t>* indexed = nullptr) const;
    void draw_small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false) const;
    s2ne::Result<bool> try_draw_small_fixed_spaces(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false) const;
    void draw_small_fixed_spaces(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, bool original_padding = false) const;
    s2ne::Result<bool> try_draw_big5_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
        int x, int y, unsigned color, std::vector<uint8_t>* indexed = nullptr) const;
    s2ne::Result<bool> try_draw_big5(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
        int x, int y, unsigned color, std::vector<uint8_t>* indexed = nullptr) const;
    void draw_big5(std::vector<uint8_t>& image, const std::vector<uint8_t>& text,
        int x, int y, unsigned color) const;
    s2ne::Result<bool> try_draw_original_large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, std::optional<unsigned> shadow = std::nullopt, std::vector<uint8_t>* indexed = nullptr) const;
    void draw_original_large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, std::optional<unsigned> shadow = std::nullopt) const;

private:
    s2ne::Result<bool> draw_original_wrapped_validated(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, unsigned rows, unsigned columns, int row_height, std::optional<unsigned> shadow) const;

public:
    s2ne::Result<bool> try_draw_original_wrapped(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, unsigned rows, unsigned columns, int row_height, std::optional<unsigned> shadow = std::nullopt) const;
    void draw_original_wrapped(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color, unsigned rows, unsigned columns, int row_height, std::optional<unsigned> shadow = std::nullopt) const;
    void draw_banner_ascii(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y) const;
    void draw(std::vector<uint8_t>& image) const;
};
}    //namespace s2ne
