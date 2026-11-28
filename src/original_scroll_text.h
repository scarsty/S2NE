#pragma once
#include "original_ending.h"

namespace s2ne
{
// 原41FA56／41F80C：独立文字层自画面底部向上滚动，返回而非终止剧情。
class OriginalScrollText
{
    OriginalMenu font_;
    std::vector<uint8_t> background_;
    std::vector<std::vector<uint8_t>> rows_;
    unsigned elapsed_{};
    int top_{ 478 }, visible_{ 2 }, source_{}, remaining_{};
    std::array<uint8_t, 3> ink_{};
    bool done_{};

public:
    OriginalScrollText(const std::filesystem::path& root, unsigned background, unsigned text, OriginalDisc disc);
    bool done() const;
    void cancel();
    void tick(unsigned milliseconds);
    std::vector<uint8_t> draw() const;
};
}    //namespace s2ne
