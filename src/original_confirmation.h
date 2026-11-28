#pragma once
#include "original_menu.h"
#include "original_palette.h"
#include "original_sprite.h"
#include <optional>
namespace s2ne
{
class OriginalConfirmation
{
    SharedBytes cel_, pal_;
    std::vector<uint8_t> text_;
    int x_, y_, selection_;
    std::optional<bool> result_;

public:
    OriginalConfirmation(const std::filesystem::path& root, const OriginalMenu& font, unsigned string_id, int x, int y, bool default_yes = true);
    void toggle();
    void confirm();
    void cancel();
    std::optional<bool> result() const;
    int selection() const;
    void mouse(float x, float y, bool left_click);
    s2ne::Result<bool> try_draw(std::vector<uint8_t>& image, const OriginalMenu& font) const;
    void draw(std::vector<uint8_t>& image, const OriginalMenu& font) const;
};
}    //namespace s2ne
