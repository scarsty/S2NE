#pragma once
#include "menu.h"
#include <cstdint>
#include <memory>
#include <string>

namespace s2ne
{
// sub_434103 / sub_432FFD. No audio device, playback or mixer calls are made here.
class Settings
{
public:
    struct State
    {
        int screen_delay = 0, message_delay = 10;    // 4F7CF0, 4F7CF4
        int midi_volume = 100, cdda_volume = 100, effect_volume = 100;
        uint8_t confirm = 1, status_window = 1, quick_switch = 0, auto_switch = 0;
        uint8_t masking = 1, animation = 0, cdda = 1;
        uint8_t flag_4f7d05 = 0, flag_4f7d09 = 1, flag_4f7d0a = 1;
        uint8_t difficulty = 3;    // 4F7CEC
        int device = 0, small_font = 0;
        char drive = '@';
        bool memory_check = false, debug_enabled = false, debug_secondary = false;
        uint8_t simplified_chinese = 0;
        bool protagonist_class_tree = false;
        bool highlight_secrets = false, gameplay_assist = false;
        int screen_width = 768, screen_height = 480;
    };
    inline static bool highlight_secrets = false, gameplay_assist = false;
    struct Event
    {
        unsigned type = 0;    // original 1/2 keyboard, 8/0x20 mouse
        int x = 0, y = 0;
        unsigned buttons = 0, scan = 0, modifiers = 0, ascii = 0;
    };
    struct Result
    {
        bool closed = false, changed = false, save_config = false;
        bool rebuild_mask_surfaces = false, debug_scene_requested = false;
        unsigned consumed_mask = 0;
    };
    static constexpr unsigned entry_address = 0x434103, page_address = 0x432FFD;
    static constexpr unsigned resource_id = 202;
    static constexpr std::array<unsigned, 7> toggle_addresses{
        0x4F7D04, 0x4F7D05, 0x4F7D06, 0x4F7D07, 0x4F7D0A, 0x4F7D09, 0x4F7D08
    };

private:
    std::vector<uint8_t> pal_, sprite_;
    Menu font_;
    State state_;
    int mode_, width_ = 0, height_ = 0, left_ = 0, top_ = 0;
    uint8_t old_mask_;
    bool closed_ = false, password_ = false;
    bool extended_ = false;
    std::string password_text_;
    size_t password_cursor_ = 0;
    std::array<uint8_t*, 7> toggles();
    std::array<uint8_t, 7> toggles() const;
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned color, unsigned alpha = 255) const;
    void box(std::vector<uint8_t>& image, int x, int y, int w, int h, unsigned c) const;
    // Glyph buffers in 42274F/423249 have one pixel of padding. Style 0
    // adds cardinal outlines; style 1 adds a (1,1) shadow; style 2 is plain.
    void text(std::vector<uint8_t>& image, const std::vector<uint8_t>& bytes, int x, int y,
        bool big, unsigned ink, unsigned edge, unsigned style) const;
    void label(std::vector<uint8_t>& image, unsigned id, int x, int y, bool big, unsigned ink, unsigned edge, unsigned style) const;
    void extension_text(std::vector<uint8_t>& image, const std::string& value, int x, int y, unsigned ink = 175) const;
    Result finish();

    struct InitializationTag
    {
    };
    Settings(Menu font, int mode, State state, InitializationTag);

    s2ne::Result<bool> initialize(const std::filesystem::path& root, int mode);

public:
    explicit Settings(const std::filesystem::path& root, int mode, State state);
    static s2ne::Result<std::unique_ptr<Settings>> create(const std::filesystem::path& root, int mode, State state);

    static State load_state(const std::filesystem::path& root);
    explicit Settings(const std::filesystem::path& root, int mode = 0);
    const State& state() const;
    bool closed() const;
    int left() const;
    int top() const;
    bool password_active() const;
    bool extended_page() const;
    static std::string config_text(const State& state_);
    std::string config_text() const;
    static s2ne::Result<bool> try_save_state(const std::filesystem::path& root, const State& state);
    // The host owns persistence: only Result::save_config permits writing this text.
    Result event(const Event& e);
    s2ne::Result<std::vector<uint8_t>> try_draw(const std::vector<uint8_t>& background) const;
    std::vector<uint8_t> draw(const std::vector<uint8_t>& background) const;
};
}    //namespace s2ne
