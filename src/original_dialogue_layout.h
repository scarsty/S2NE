#pragma once
#include <algorithm>
namespace s2ne
{
struct OriginalDialogueLayout
{
    int frame_y, arrow_x, arrow_y;
    unsigned arrow_resource;
    // Raw CEL pixels: 237 has its tip at bottom-right; 238 at top-left.
    // Keep the 41CBFF/41CC44 anchors, with the tip facing the speaker.
    static OriginalDialogueLayout locate(int screen_x, int screen_y, int height)
    {
        bool above = screen_y > 310;
        int y = above ? std::max(32, screen_y - 100 - height) : std::min(screen_y - 2, 479 - height);
        return { y, std::clamp(screen_x + (above ? 12 : 33), 0, 591), above ? y + 215 : y - 32, above ? 237u : 238u };
    }
};
}    //namespace s2ne
