#pragma once
#include "menu.h"
#include "palette.h"
#include "sprite.h"
#include <optional>
namespace s2ne
{
class Confirmation
{
    SharedBytes cel_, pal_;
    std::vector<uint8_t> text_;
    int x_, y_, selection_;
    std::optional<bool> result_;

public:
    Confirmation(const std::filesystem::path& root, const Menu& font, unsigned string_id, int x, int y, bool default_yes = true);
    void toggle();
    void confirm();
    void cancel();
    std::optional<bool> result() const;
    int selection() const;
    void mouse(float x, float y, bool left_click);
    s2ne::Result<bool> try_draw(std::vector<uint8_t>& image, const Menu& font) const;
    void draw(std::vector<uint8_t>& image, const Menu& font) const;
};
}    //namespace s2ne
