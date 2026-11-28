#pragma once
#include "original_menu.h"
#include "original_sprite.h"
#include "title_image.h"
namespace s2ne
{
class NewGameSetup
{
    std::vector<uint8_t> dat, pal, backdrop_;
    size_t group{};
    Result<bool> try_sprite(std::vector<uint8_t>& image, unsigned id, int left, int top) const;

    void sprite(std::vector<uint8_t>& image, unsigned id, int left, int top) const;

public:
    int difficulty = 2;
    unsigned confirmed = 0;
    NewGameSetup() = default;
    static Result<NewGameSetup> create(const std::filesystem::path& root);
    explicit NewGameSetup(const std::filesystem::path& root);
    void move(int d);
    bool mouse(float x, float y, bool left_click);
    void confirm();
    std::vector<uint8_t> draw(const std::vector<uint8_t>& background, const OriginalMenu& font) const;
};
}    //namespace s2ne
