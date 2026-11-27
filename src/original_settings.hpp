#pragma once
#include "original_menu.hpp"
#include "original_save_store.hpp"
#include "original_sprite.hpp"
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>

namespace s2ne
{
// sub_434103 / sub_432FFD. No audio device, playback or mixer calls are made here.
class OriginalSettings
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
        int screen_width = 768, screen_height = 480;
    };
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
    OriginalMenu font_;
    State state_;
    int mode_, width_ = 0, height_ = 0, left_ = 0, top_ = 0;
    uint8_t old_mask_;
    bool closed_ = false, password_ = false;
    std::string password_text_;
    size_t password_cursor_ = 0;
    std::array<uint8_t*, 7> toggles() { return { &state_.masking, &state_.flag_4f7d05, &state_.animation, &state_.status_window, &state_.flag_4f7d0a, &state_.flag_4f7d09, &state_.cdda }; }
    std::array<uint8_t, 7> toggles() const { return { state_.masking, state_.flag_4f7d05, state_.animation, state_.status_window, state_.flag_4f7d0a, state_.flag_4f7d09, state_.cdda }; }
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned color, unsigned alpha = 255) const
    {
        if (x < 0 || x >= 640 || y < 0 || y >= 480)
        {
            return;
        }
        size_t p = (size_t(y) * 640 + x) * 4;
        for (unsigned c = 0; c < 3; ++c)
        {
            const unsigned ink = std::min(255u, unsigned(pal_.at(color * 3 + c)) * 255 / 63);
            image.at(p + c) = uint8_t((ink * alpha + unsigned(image.at(p + c)) * (255 - alpha) + 127) / 255);
        }
        image.at(p + 3) = 255;
    }
    void box(std::vector<uint8_t>& image, int x, int y, int w, int h, unsigned c) const
    {
        for (int yy = 0; yy < h; ++yy)
        {
            for (int xx = 0; xx < w; ++xx)
            {
                pixel(image, x + xx, y + yy, c);
            }
        }
    }
    // Glyph buffers in 42274F/423249 have one pixel of padding. Style 0
    // adds cardinal outlines; style 1 adds a (1,1) shadow; style 2 is plain.
    void text(std::vector<uint8_t>& image, const std::vector<uint8_t>& bytes, int x, int y,
        bool big, unsigned ink, unsigned edge, unsigned style) const
    {
        for (size_t p = 0; p < bytes.size();)
        {
            uint8_t lead = bytes.at(p++);
            if (!lead || lead == 10)
            {
                break;
            }
            std::vector<uint8_t> ch{ lead };
            if (lead > 128)
            {
                ch.push_back(bytes.at(p++));
            }
            bool large = big && (!state_.small_font || lead <= 128);
            int gx = x + 1 + (big && lead <= 128 ? 2 : 0) + (big && state_.small_font && lead > 128 ? 4 : 0);
            int gy = y + (lead <= 128 ? 1 : 0) + (big && state_.small_font && lead > 128 ? 4 : 0);
            std::vector<uint8_t> mask(640 * 480 * 4);
            if (large)
            {
                font_.draw_big5(mask, ch, gx, gy, 1);
            }
            else
            {
                font_.draw_small(mask, ch, gx, gy, 1);
            }
            int glyphWidth;
            if (lead > 128)
            {
                if (large) { glyphWidth = 24; }
                else
                {
                    glyphWidth = 16;
                }
            }
            else if (large) { glyphWidth = 16; }
            else
            {
                glyphWidth = 8;
            }
            int glyphHeight;
            if (large) { glyphHeight = 24; }
            else if (lead > 128) { glyphHeight = 16; }
            else
            {
                glyphHeight = 15;
            }

            int w = glyphWidth, h = glyphHeight;
            std::vector<uint8_t> outline(size_t(w + 2) * (h + 2));
            for (int yy = gy; yy < gy + h; ++yy)
            {
                for (int xx = gx; xx < gx + w; ++xx)
                {
                    if (xx < 0 || xx >= 640 || yy < 0 || yy >= 480 || !mask[(size_t(yy) * 640 + xx) * 4 + 3])
                    {
                        continue;
                    }
                    if (style == 1)
                    {
                        outline[size_t(yy - gy + 2) * (w + 2) + xx - gx + 2] = mask[(size_t(yy) * 640 + xx) * 4 + 3];
                    }
                    else if (style == 0)
                    {
                        const auto alpha = mask[(size_t(yy) * 640 + xx) * 4 + 3];
                        for (const auto [dx, dy] : std::array<std::pair<int, int>, 4>{ { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } })
                        {
                            auto& coverage = outline[size_t(yy - gy + 1 + dy) * (w + 2) + xx - gx + 1 + dx];
                            coverage = std::max(coverage, alpha);
                        }
                    }
                }
            }
            for (int yy = 0; yy < h + 2; ++yy)
            {
                for (int xx = 0; xx < w + 2; ++xx)
                {
                    const auto alpha = outline[size_t(yy) * (w + 2) + xx];
                    if (alpha) { pixel(image, gx + xx - 1, gy + yy - 1, edge, alpha); }
                }
            }
            for (int yy = gy; yy < gy + h; ++yy)
            {
                for (int xx = gx; xx < gx + w; ++xx)
                {
                    if (xx >= 0 && xx < 640 && yy >= 0 && yy < 480 && mask[(size_t(yy) * 640 + xx) * 4 + 3])
                    {
                        pixel(image, xx, yy, ink, mask[(size_t(yy) * 640 + xx) * 4 + 3]);
                    }
                }
            }
            int glyphAdvance;
            if (big)
            {
                if (lead > 128) { glyphAdvance = 24; }
                else
                {
                    glyphAdvance = 16;
                }
            }
            else if (lead > 128) { glyphAdvance = 16; }
            else
            {
                glyphAdvance = 8;
            }

            x += glyphAdvance;
        }
    }
    void label(std::vector<uint8_t>& image, unsigned id, int x, int y, bool big, unsigned ink, unsigned edge, unsigned style) const
    {
        text(image, font_.text_bytes(id), left_ + x, top_ + y, big, ink, edge, style);
    }
    Result finish()
    {
        closed_ = true;
        return { true, false, mode_ != 2, mode_ == 1 && state_.status_window != old_mask_, false, 0x1F };
    }

    struct InitializationTag
    {
    };
    OriginalSettings(OriginalMenu font, int mode, State state, InitializationTag) : font_(std::move(font)), state_(state), mode_(mode), old_mask_(state.status_window) {}

    s2ne::Result<bool> initialize(const std::filesystem::path& root, int mode)
    {
        if (mode < 0 || mode > 2)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid original settings context"));
        }
        auto pal_read_result = read_resource(root / "SUPERSP.PAL");
        if (!pal_read_result)
        {
            return s2ne::Result<bool>::failure(pal_read_result.error());
        }
        pal_ = std::move(pal_read_result).value();
        if (pal_.size() < 820) { return s2ne::Result<bool>::failure(make_error("Truncated settings palette")); }
        pal_.at(0) = pal_.at(1) = pal_.at(2) = 0;
        auto cel_read_result = read_shared_resource(root / "SUPERSP.CEL");
        if (!cel_read_result)
        {
            return s2ne::Result<bool>::failure(cel_read_result.error());
        }
        const auto cel = std::move(cel_read_result).value();
        // 425E84: CEL correction starts at 56; 426CB0 maps 202 to index 60.
        auto address = dword(pal_, 808 + 8);
        if (address < 57) { return s2ne::Result<bool>::failure(make_error("Invalid settings CEL address")); }
        size_t group = address - 57;
        if (group > cel.size() || cel.size() - group < 2 + 61 * 8) { return s2ne::Result<bool>::failure(make_error("Truncated settings CEL directory")); }
        if (word(cel, group) <= 60)
        {
            return s2ne::Result<bool>::failure(make_error("Missing CEL2 settings panel"));
        }
        size_t q = group + 2 + 60 * 8, offset = group + dword(cel, q), size = dword(cel, q + 4);
        if (offset > cel.size() || size > cel.size() - offset || size < 4)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid CEL2 settings panel"));
        }
        sprite_.assign(cel.begin() + offset, cel.begin() + offset + size);
        width_ = word(sprite_, 0) + 1;
        height_ = word(sprite_, 2) + 1;
        if (width_ > 640 || height_ > 480)
        {
            return s2ne::Result<bool>::failure(make_error("Invalid settings panel dimensions"));
        }
        left_ = 320 - width_ / 2;
        top_ = 240 - height_ / 2;
        size_t p = 4;
        for (int y = 0; y < height_; ++y)
        {
            for (int x = 0; x < width_;)
            {
                if (p >= sprite_.size()) { return s2ne::Result<bool>::failure(make_error("Truncated settings panel opcode")); }
                unsigned code = sprite_[p++], n = code ? code & 127 : 1;
                if (!n || n > unsigned(width_ - x))
                {
                    return s2ne::Result<bool>::failure(make_error("Invalid settings panel run"));
                }
                if (code)
                {
                    p += (code & 128) ? n : 1;
                }
                if (p > sprite_.size())
                {
                    return s2ne::Result<bool>::failure(make_error("Truncated settings panel run"));
                }
                x += int(n);
            }
        }
        if (p != sprite_.size())
        {
            return s2ne::Result<bool>::failure(make_error("Trailing settings panel bytes"));
        }

        return s2ne::Result<bool>::success(true);
    }

public:
    explicit OriginalSettings(const std::filesystem::path& root, int mode, State state) : OriginalSettings(std::move(*create(root, mode, state).value())) {}
    static s2ne::Result<std::unique_ptr<OriginalSettings>> create(const std::filesystem::path& root, int mode, State state)
    {
        if (mode < 0 || mode > 2) { return s2ne::Result<std::unique_ptr<OriginalSettings>>::failure(make_error("Invalid original settings context")); }
        auto font = OriginalMenu::create(root);
        if (!font) { return s2ne::Result<std::unique_ptr<OriginalSettings>>::failure(font.error()); }
        auto result = std::unique_ptr<OriginalSettings>(new OriginalSettings(std::move(font).value(), mode, state, InitializationTag{}));
        auto ready = result->initialize(root, mode);
        if (!ready)
        {
            return s2ne::Result<std::unique_ptr<OriginalSettings>>::failure(ready.error());
        }
        return s2ne::Result<std::unique_ptr<OriginalSettings>>::success(std::move(result));
    }

    static State load_state(const std::filesystem::path& root)
    {
        State state;
        std::ifstream f(root / "SUPERSP.CFG");
        std::string line;
        while (std::getline(f, line))
        {
            int a, b, c, d, e, g, h, i;
            char drive;
            const auto separator = line.find('=');
            std::istringstream values(separator == std::string::npos ? "" : line.substr(separator + 1));
            auto integer = [&values](int& value, bool comma)
            {
                if (!(values >> value)) { fatal("Malformed or out-of-range CFG integer"); }
                if (comma)
                {
                    char delimiter{};
                    if (!(values >> delimiter) || delimiter != ',') { fatal("Malformed CFG separator"); }
                }
            };
            if (line.starts_with("PROTAGONIST_CLASS_TREE="))
            {
                integer(a, false);
                if (a != 0 && a != 1) { fatal("CFG PROTAGONIST_CLASS_TREE must be0 or1"); }
                state.protagonist_class_tree = a != 0;
            }
            else if (line.starts_with("RESOLUTION="))
            {
                integer(a, true);
                integer(b, false);
                if (a < 640 || a > 1920 || b < 480 || b > 1440) { fatal("CFG RESOLUTION must be within 640x480 and 1920x1440"); }
                state.screen_width = a;
                state.screen_height = b;
            }
            else if (line.starts_with("DEVICE="))
            {
                integer(a, true);
                integer(b, true);
                if (!(values >> drive)) { fatal("Missing CFG drive"); }
                if (a < 0 || (b != 0 && b != 1)) { fatal("Invalid CFG DEVICE values"); }
                state.device = a;
                state.small_font = b;
                state.drive = drive;
            }
            else if (line.starts_with("VOLUME="))
            {
                integer(a, true);
                integer(b, true);
                integer(c, false);
                if (a < 0 || a > 100 || b < 0 || b > 100 || c < 0 || c > 100) { fatal("Invalid CFG VOLUME values"); }
                state.midi_volume = a;
                state.cdda_volume = b;
                state.effect_volume = c;
            }
            else if (line.starts_with("CDDA="))
            {
                std::string setting;
                values >> setting;
                if (setting != "ON" && setting != "OFF") { fatal("Invalid CFG CDDA value"); }
                state.flag_4f7d05 = setting == "ON";
            }
            else if (line.starts_with("SET="))
            {
                integer(a, true);
                integer(b, true);
                integer(c, true);
                integer(d, true);
                integer(e, true);
                integer(g, true);
                integer(h, true);
                integer(i, false);
                if (a < 0 || b < 0 || a > int(UINT32_MAX / 10) || b > int(UINT32_MAX / 10)
                    || c < 0 || c > 1 || d < 0 || d > 1 || e < 0 || e > 1 || g < 0 || g > 1 || h < 0 || h > 1 || i < 0 || i > 1)
                {
                    fatal("Invalid CFG SET values");
                }
                state.screen_delay = a;
                state.message_delay = b;
                state.confirm = uint8_t(c);
                state.status_window = uint8_t(d);
                state.quick_switch = uint8_t(e);
                state.auto_switch = uint8_t(g);
                state.animation = uint8_t(h);
                state.masking = uint8_t(i);
            }
            else if (line == "MemoryCheck=ON" || line == "MemoryCheck=ON\r")
            {
                state.memory_check = true;
            }
            else if (line == "SIMPLIFIED_CHINESE=1" || line == "SIMPLIFIED_CHINESE=ON") { state.simplified_chinese = 1; }
            if (line.starts_with("PROTAGONIST_CLASS_TREE=") || line.starts_with("RESOLUTION=") || line.starts_with("DEVICE=") || line.starts_with("VOLUME=") || line.starts_with("SET=") || line.starts_with("CDDA="))
            {
                values >> std::ws;
                if (!values.eof()) { fatal("Trailing original CFG setting data"); }
            }
        }
        if (f.bad()) { fatal("Cannot finish reading original CFG"); }
        return state;
    }
    explicit OriginalSettings(const std::filesystem::path& root, int mode = 0) : OriginalSettings(root, mode, load_state(root)) {}
    const State& state() const { return state_; }
    bool closed() const { return closed_; }
    int left() const { return left_; }
    int top() const { return top_; }
    bool password_active() const { return password_; }
    static std::string config_text(const State& state_)
    {
        std::ostringstream o;
        o << "RESOLUTION=" << state_.screen_width << ',' << state_.screen_height << '\n';
        o << "PROTAGONIST_CLASS_TREE=" << int(state_.protagonist_class_tree) << '\n';
        o << "DEVICE=" << state_.device << ',' << state_.small_font << ',' << state_.drive << "\nVOLUME="
          << state_.midi_volume << ',' << state_.cdda_volume << ',' << state_.effect_volume
          << "\nCDDA=" << (state_.flag_4f7d05 ? "ON" : "OFF") << "\nSET=" << state_.screen_delay << ',' << state_.message_delay
          << ',' << unsigned(state_.confirm) << ',' << unsigned(state_.status_window) << ',' << unsigned(state_.quick_switch)
          << ',' << unsigned(state_.auto_switch) << ',' << unsigned(state_.animation) << ',' << unsigned(state_.masking) << "\nSIMPLIFIED_CHINESE=" << unsigned(state_.simplified_chinese) << "\nVersion=1.3d\n";
        if (state_.memory_check)
        {
            o << "MemoryCheck=ON\n";
        }
        return o.str();
    }
    std::string config_text() const { return config_text(state_); }
    static s2ne::Result<bool> try_save_state(const std::filesystem::path& root, const State& state)
    {
        std::map<std::string, std::string> replacements;
        std::istringstream updated(config_text(state));
        std::string line;
        while (std::getline(updated, line))
        {
            const auto eq = line.find('=');
            if (eq != std::string::npos && line.substr(0, eq) != "Version") { replacements[line.substr(0, eq)] = line; }
        }
        const auto path = root / "SUPERSP.CFG";
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);
        if (error) { return s2ne::Result<bool>::failure(make_error("Cannot inspect original CFG: " + error.message())); }
        std::ifstream existing;
        if (exists)
        {
            existing.open(path, std::ios::binary);
            if (!existing) { return s2ne::Result<bool>::failure(make_error("Cannot read original CFG")); }
        }
        std::set<std::string> emitted;
        std::string merged;
        while (std::getline(existing, line))
        {
            if (!line.empty() && line.back() == '\r') { line.pop_back(); }
            const auto key = line.substr(0, line.find('='));
            auto replacement = replacements.find(key);
            if (replacement != replacements.end())
            {
                if (!emitted.insert(key).second) { continue; }
                line = replacement->second;
            }
            merged += line + "\r\n";
        }
        if (existing.bad()) { return s2ne::Result<bool>::failure(make_error("Cannot finish reading original CFG")); }
        existing.close();
        for (const auto& [key, value] : replacements)
        {
            if (!emitted.contains(key)) { merged += value + "\r\n"; }
        }
        return OriginalSaveStore::try_replace(path, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(merged.data()), merged.size()));
    }
    // The host owns persistence: only Result::save_config permits writing this text.
    Result event(const Event& e)
    {
        if (closed_)
        {
            return { true };
        }
        Result r;
        if (password_)
        {
            if (e.type != 1 && e.type != 2)
            {
                return r;
            }
            unsigned c = e.ascii & 255;
            if (c == 27)
            {
                password_ = false;
                r.changed = true;
            }
            else if (c == 13 && !password_text_.empty())
            {
                auto lower = password_text_;
                for (char& ch : lower)
                {
                    if (ch >= 'A' && ch <= 'Z')
                    {
                        ch = char(ch - 'A' + 'a');
                    }
                }
                // 419876 subtracts i+1 from its two embedded byte sequences.
                if (lower == "ujgame")
                {
                    state_.debug_enabled = true;
                }
                if (lower == "uj141421")
                {
                    state_.debug_secondary = true;
                }
                password_ = false;
                r.changed = true;
            }
            else if (c == 8)
            {
                if (password_cursor_)
                {
                    password_text_.erase(--password_cursor_, 1);
                    r.changed = true;
                }
            }
            else if (c && c != 13)
            {
                if (password_cursor_ < password_text_.size())
                {
                    password_text_[password_cursor_] = char(c);
                }
                else
                {
                    password_text_.push_back(char(c));
                }
                if (password_cursor_ < 9)
                {
                    ++password_cursor_;
                }
                r.changed = true;
            }
            return r;
        }
        if (e.type == 1 || e.type == 2)
        {
            if (e.scan == 1)
            {
                return finish();
            }
            if (e.scan == 0x18 && (e.modifiers & 0x60))
            {
                password_ = true;
                password_text_.clear();
                password_cursor_ = 0;
                r.changed = true;
                r.consumed_mask = 4;
            }
            if (e.scan == 0x2E && state_.debug_enabled && (e.modifiers & 0x40))
            {
                r.debug_scene_requested = true;
            }
            return r;
        }
        if (e.type != 8 && e.type != 0x20)
        {
            return r;
        }
        if (e.buttons & 1)
        {
            r.consumed_mask = 0x10;
            int x = e.x - left_, y = e.y - top_;
            if (x >= 244 && x < 484 && y >= 47 && y <= 160)
            {
                int step = (x - 244) / 24;
                if ((x - 244) % 24 >= 19)
                {
                    return r;
                }
                int row = -1;
                for (int i = 0; i < 4; ++i)
                {
                    if (y >= 47 + i * 30 && y <= 70 + i * 30)
                    {
                        row = i;
                        break;
                    }
                }
                int *dst = nullptr, value = 0;
                if (row == 0)
                {
                    dst = &state_.screen_delay;
                    value = (9 - step) * 3;
                }
                if (row == 1)
                {
                    dst = &state_.message_delay;
                    value = (9 - step) * 5;
                }
                if (row == 2)
                {
                    dst = state_.cdda ? &state_.cdda_volume : &state_.midi_volume;
                    value = step == 9 ? 100 : step * 11;
                }
                if (row == 3)
                {
                    dst = &state_.effect_volume;
                    value = step == 9 ? 100 : step * 11;
                }
                if (dst && *dst != value)
                {
                    *dst = value;
                    r.changed = true;
                }
                return r;
            }
            if (x >= 191 && x <= 306)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (y >= 174 + i * 30 && y <= 197 + i * 30)
                    {
                        *toggles()[i] ^= 1;
                        r.changed = true;
                        return r;
                    }
                }
            }
            if (x >= 431 && x <= 546)
            {
                for (int i = 0; i < 3; ++i)
                {
                    if (y >= 174 + i * 30 && y <= 197 + i * 30)
                    {
                        *toggles()[4 + i] ^= 1;
                        r.changed = true;
                        return r;
                    }
                }
                if (mode_ == 2 && y >= 264 && y <= 287)
                {
                    if (x <= 477 && int8_t(state_.difficulty) > 1)
                    {
                        --state_.difficulty;
                        r.changed = true;
                    }
                    else if (x >= 497 && int8_t(state_.difficulty) < 5)
                    {
                        ++state_.difficulty;
                        r.changed = true;
                    }
                }
            }
        }
        else if (e.buttons & 2)
        {
            return finish();
        }
        return r;
    }
    s2ne::Result<std::vector<uint8_t>> try_draw(const std::vector<uint8_t>& background) const
    {
        if (background.size() != 640 * 480 * 4)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Settings requires a 640x480 RGBA backdrop"));
        }
        auto image = background;
        auto panel = draw_original_sprite(image, sprite_, pal_, 0, false, left_, top_, OriginalZeroRun::transparent);
        if (!panel)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(panel.error());
        }
        label(image, 130, 262, 12, true, 73, 55, 0);
        for (int i = 0; i < 4; ++i)
        {
            label(image, 132 + i, 78, 44 + i * 31, true, 51, 41, 1);
            label(image, 136 + i, 78, 173 + i * 31, true, 51, 41, 1);
            label(image, 140 + i, 319, 173 + i * 31, true, 51, 41, 1);
        }
        for (int i = 0; i < 4; ++i)
        {
            label(image, i < 2 ? 146 : 148, 210, 47 + i * 31, false, 175, 181, 0);
            label(image, i < 2 ? 147 : 149, 504, 47 + i * 31, false, 175, 181, 0);
        }
        unsigned difficulty_ink = mode_ == 2 ? 175 : 51, difficulty_edge = mode_ == 2 ? 181 : 41;
        label(image, 152, 441, 269, false, difficulty_ink, difficulty_edge, 1);
        label(image, 153, 515, 269, false, difficulty_ink, difficulty_edge, 1);
        std::string number = std::to_string(int(int8_t(state_.difficulty)));
        text(image, { number.begin(), number.end() }, left_ + 482, top_ + 269, false, difficulty_ink, difficulty_edge, 1);
        const std::array<int, 4> values{ state_.screen_delay / 3, state_.message_delay / 5,
            (state_.cdda ? state_.cdda_volume : state_.midi_volume) / 11, state_.effect_volume / 11 };
        for (int i = 0; i < 4; ++i)
        {
            int s = std::min(9, values[i]);
            if (i < 2)
            {
                s = 9 - s;
            }
            box(image, left_ + 247 + s * 24, top_ + 52 + i * 31, 14, 8, 189);
        }
        const auto flags = toggles();
        for (int i = 0; i < 7; ++i)
        {
            int col = i < 4 ? 0 : 1, row = i < 4 ? i : i - 4;
            if (flags[i] == 1)
            {
                box(image, left_ + 218 + 240 * col, top_ + 183 + 31 * row, 14, 8, 189);
            }
            label(image, flags[i] == 1 ? 150 : 151, 250 + 240 * col, 177 + 31 * row, false, 175, 181, 0);
        }
        if (password_)
        {
            box(image, 0, 450, 560, 30, 0);
            std::string prompt = "PassWord:";
            text(image, { prompt.begin(), prompt.end() }, 0, 452, true, 254, 0, 2);
            text(image, { password_text_.begin(), password_text_.end() }, 144, 452, true, 254, 0, 2);
            text(image, { '_' }, 144 + int(password_cursor_) * 16, 452, true, 254, 0, 2);
        }
        return s2ne::Result<std::vector<uint8_t>>::success(image);
    }
    std::vector<uint8_t> draw(const std::vector<uint8_t>& background) const
    {
        auto result = try_draw(background);
        return std::move(result).value();
    }
};
}    //namespace s2ne
