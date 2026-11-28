#pragma once
#include "menu.h"
#include "save.h"
#include "sprite.h"
#include <memory>
#include <optional>
#include <string>

namespace s2ne
{
// sub_4354C0: selector only. Caller performs confirmation and actual save/load.
class SaveMenu
{
public:
    struct Slot
    {
        std::array<uint8_t, 50> description{};
        bool occupied() const;
    };
    using Slots = std::array<Slot, 53>;
    enum class Action
    {
        none,
        load,
        save,
        confirm_overwrite,
        selected,
        cancel
    };
    struct Result
    {
        Action action = Action::none;
        unsigned slot = 0;
        bool changed = false;
    };
    struct Event
    {
        unsigned type = 0;
        int x = 0, y = 0;
        unsigned buttons = 0, ascii = 0, scan = 0;
    };
    static constexpr unsigned address = 0x4354C0, resource_id = 204;
    static constexpr std::array<unsigned, 4> title_ids{ 128, 129, 271, 272 };

private:
    SharedBytes cel_, pal_;
    Slots slots_;
    int mode_, page_, row_ = 0, left_ = 0, top_ = 0, width_ = 0, height_ = 0;
    bool closed_ = false;
    std::optional<Result> pending_;
    int return_code_ = 0;
    static s2ne::Result<SharedBytes> try_read(const std::filesystem::path& path);
    size_t sprite_position() const;
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned c) const;
    s2ne::Result<bool> try_panel(std::vector<uint8_t>& image) const;
    void panel(std::vector<uint8_t>& image) const;
    unsigned slot_for(int row) const;
    bool occupied(int row) const;
    void seek_down();
    Result activate();

public:
    static Slots scan(const std::filesystem::path& root);

private:
    SaveMenu(SharedBytes cel, SharedBytes pal, int mode, Slots slots, int page);
    s2ne::Result<bool> initialize(bool original_mode_one);

public:
    SaveMenu(const std::filesystem::path& root, int mode, Slots slots, int page = 0, bool original_mode_one = false);
    static s2ne::Result<std::unique_ptr<SaveMenu>> create(const std::filesystem::path& root, int mode, Slots slots, int page = 0, bool original_mode_one = false);

    int left() const;
    int top() const;
    int width() const;
    int height() const;
    int page() const;
    int row() const;
    unsigned selected_slot() const;
    bool closed() const;
    int return_code() const;
    const Slots& slots() const;
    int hit(int x, int y) const;
    Result command(unsigned scan);
    Result event(const Event& e);
    Result confirm_overwrite(bool yes);
    // Original closes after its synchronous IO. Adapter reports failure without claiming success.
    s2ne::Result<bool> try_complete(bool success);
    void complete(bool success);
    s2ne::Result<bool> try_draw_validated(std::vector<uint8_t>& image, const Menu& font) const;
    s2ne::Result<bool> try_draw(std::vector<uint8_t>& image, const Menu& font) const;
    void draw(std::vector<uint8_t>& image, const Menu& font) const;
};
}    //namespace s2ne
