#pragma once
#include "ani_audio.h"
#include "campaign.h"
#include "disc.h"
#include "events.h"
#include "flic.h"
#include "menu.h"
#include <memory>
#include <optional>
namespace s2ne
{
// 41E9CF resource order and timed presentation; no save flag is manufactured.
class Ending
{
    Menu font_;
    SharedBytes pal_, cel_, prg_;
    std::vector<uint8_t> text_, background_, intro_;
    s2ne::Result<bool> try_class_image_validated(std::vector<uint8_t>& image, unsigned appearance) const;
    s2ne::Result<bool> try_class_image(std::vector<uint8_t>& image, unsigned appearance) const;
    void class_image(std::vector<uint8_t>& image, unsigned appearance) const;
    Events events_;
    std::vector<unsigned> roles_;
    size_t page_{};
    unsigned variant_{};
    std::vector<std::vector<uint8_t>> illustrations_;
    std::vector<unsigned> illustration_pages_;
    unsigned scroll_{}, elapsed_{}, movie_frames_{};
    std::optional<Flic> movie_;
    std::optional<AniAudio> movie_audio_;
    AniAudio::Play play_audio_;
    AniAudio::Stop stop_audio_;
    AniAudio::Music music_audio_;
    AniAudio::Music midi_audio_;
    std::filesystem::path root_;
    Disc disc_;
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
    static s2ne::Result<std::vector<uint8_t>> try_pcx(const std::filesystem::path& root, unsigned id, Disc disc);

private:
    std::vector<uint8_t> pcx(const std::filesystem::path& root, unsigned id, Disc disc);
    s2ne::Result<bool> paragraph(std::vector<uint8_t>& image, const std::vector<uint8_t>& bytes, int x, int y, int width) const;
    s2ne::Result<bool> role_text(std::vector<uint8_t>& image, const std::vector<uint8_t>& text) const;

private:
    Ending(Menu font, const std::filesystem::path& root, const Events& events, Disc disc);
    s2ne::Result<bool> initialize(const std::filesystem::path& root, const Events& events, Disc disc);

public:
    Ending(const std::filesystem::path& root, const Events& events, Disc disc);
    static s2ne::Result<std::unique_ptr<Ending>> create(const std::filesystem::path& root, const Events& events, Disc disc);

    size_t page() const;
    size_t pages() const;
    unsigned variant() const;
    bool done() const;
    bool automatic() const;
    unsigned movie_frames() const;
    void set_effect_audio(const std::filesystem::path& root, AniAudio::Play play, AniAudio::Stop stop);
    void set_music_audio(AniAudio::Music play, AniAudio::Music midi = {});
    std::string phase_description() const;
    void tick(unsigned milliseconds);
    void advance();
    s2ne::Result<std::vector<uint8_t>> try_draw() const;
    std::vector<uint8_t> draw() const;
};
}    //namespace s2ne
