#pragma once
#include "flic.h"
#include "original_ani_audio.h"
#include "original_campaign.h"
#include "original_disc.h"
#include "original_events.h"
#include "original_menu.h"
#include <memory>
#include <optional>
namespace s2ne
{
// 41E9CF resource order and timed presentation; no save flag is manufactured.
class OriginalEnding
{
    OriginalMenu font_;
    SharedBytes pal_, cel_, prg_;
    std::vector<uint8_t> text_, background_, intro_;
    s2ne::Result<bool> try_class_image_validated(std::vector<uint8_t>& image, unsigned appearance) const;
    s2ne::Result<bool> try_class_image(std::vector<uint8_t>& image, unsigned appearance) const;
    void class_image(std::vector<uint8_t>& image, unsigned appearance) const;
    OriginalEvents events_;
    std::vector<unsigned> roles_;
    size_t page_{};
    unsigned variant_{};
    std::vector<std::vector<uint8_t>> illustrations_;
    std::vector<unsigned> illustration_pages_;
    unsigned scroll_{}, elapsed_{}, movie_frames_{};
    std::optional<Flic> movie_;
    std::optional<OriginalAniAudio> movie_audio_;
    OriginalAniAudio::Play play_audio_;
    OriginalAniAudio::Stop stop_audio_;
    OriginalAniAudio::Music music_audio_;
    OriginalAniAudio::Music midi_audio_;
    std::filesystem::path root_;
    OriginalDisc disc_;
    std::vector<uint8_t> movie_image_;
    struct Page
    {
        int role{ -1 };
        int illustration{ -1 };
        unsigned movie{}, text{};
        bool relationship{}, summary{};
    };
    std::vector<Page> sequence_;
    bool role_music_started_{};
    void enter_music();
    void enter();
    bool scrolling() const;
    static s2ne::Result<unsigned> try_word(const auto& b, size_t p)
    {
        if (p >= b.size() || b.size() - p < 2)
        {
            return s2ne::Result<unsigned>::failure(make_error("Ending word offset=" + std::to_string(p) + " size=" + std::to_string(b.size())));
        }
        return s2ne::Result<unsigned>::success(b.at(p) | (unsigned(b.at(p + 1)) << 8));
    }
    static unsigned word(const auto& b, size_t p) { return try_word(b, p).value(); }
    static unsigned dword(const auto& b, size_t p) { return word(b, p) | (word(b, p + 2) << 16); }
    std::vector<uint8_t> string(size_t p) const;

public:
    static s2ne::Result<std::vector<uint8_t>> try_pcx(const std::filesystem::path& root, unsigned id, OriginalDisc disc);

private:
    std::vector<uint8_t> pcx(const std::filesystem::path& root, unsigned id, OriginalDisc disc);
    s2ne::Result<bool> paragraph(std::vector<uint8_t>& image, const std::vector<uint8_t>& bytes, int x, int y, int width) const;
    s2ne::Result<bool> role_text(std::vector<uint8_t>& image, const std::vector<uint8_t>& text) const;

private:
    OriginalEnding(OriginalMenu font, const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc);
    s2ne::Result<bool> initialize(const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc);

public:
    OriginalEnding(const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc);
    static s2ne::Result<std::unique_ptr<OriginalEnding>> create(const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc);

    size_t page() const;
    size_t pages() const;
    unsigned variant() const;
    bool done() const;
    bool automatic() const;
    unsigned movie_frames() const;
    void set_effect_audio(const std::filesystem::path& root, OriginalAniAudio::Play play, OriginalAniAudio::Stop stop);
    void set_music_audio(OriginalAniAudio::Music play, OriginalAniAudio::Music midi = {});
    std::string phase_description() const;
    void tick(unsigned milliseconds);
    void advance();
    s2ne::Result<std::vector<uint8_t>> try_draw() const;
    std::vector<uint8_t> draw() const;
};
}    //namespace s2ne
