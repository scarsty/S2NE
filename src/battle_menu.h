#pragma once
#include "confirmation.h"
#include "settings.h"
#include "sprite.h"
#include <memory>

namespace s2ne
{
// UI/state only. World mutation and the modal operations below belong to the host.
class BattleMenu
{
public:
    enum class Page
    {
        root,
        overview,
        system,
        confirmation,
        settings,
        operation,
        closed
    };
    enum class Action
    {
        none,
        conditions,
        inspect_characters,
        inspect_map,
        load,
        save
    };
    struct Context
    {
        unsigned byte_4f6f4d = 0;
        int dword_4f7cd0 = 0;
        bool saves_present = false;
    };
    struct Request
    {
        Action action = Action::none;
        unsigned address = 0;
        int eax = 0, edx = 0, ebx = 0;
        unsigned eax_sprite = 0;
    };
    struct Result
    {
        bool changed = false;
        unsigned consumed_mask = 0;
        bool rebuild_mask_surfaces = false, debug_scene_requested = false;
        bool save_config = false;
    };
    static constexpr unsigned entry_address = 0x431DE8;
    static constexpr std::array<unsigned, 6> root_ids{ 121, 122, 123, 124, 125, 126 };
    static constexpr std::array<unsigned, 2> overview_ids{ 156, 157 };
    static constexpr std::array<unsigned, 4> system_ids{ 128, 129, 130, 131 };

private:
    std::filesystem::path root_;
    Menu font_;
    SharedBytes pal_, cel_;
    Palette palette_;
    Context context_;
    Settings::State settings_state_;
    Page page_ = Page::root, confirmation_parent_ = Page::root, operation_parent_ = Page::root;
    int selected_ = 0, root_selected_ = 0, system_selected_ = 0;
    int left_ = 0, top_ = 0, confirmation_code_ = 0;
    unsigned confirmation_id_ = 0;
    Request request_;
    std::optional<int> return_code_;
    std::unique_ptr<Confirmation> confirmation_;
    std::unique_ptr<Settings> settings_;
    static SharedBytes read_palette(const std::filesystem::path& p);
    size_t sprite_offset(unsigned id) const;
    void sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, bool transparent) const;
    void close(int code);
    void operation(Action action, unsigned address, int eax = 0, int edx = 0, int ebx = 0);
    void ask(unsigned id, int code, int x, int y, bool forced);
    void finish_confirmation();
    unsigned label_id(int row, Page page) const;
    void draw_page(std::vector<uint8_t>& image, Page page, int selection, bool draw_labels = true, bool draw_pressed = false) const;

public:
    BattleMenu(const std::filesystem::path& root, Context context, Settings::State state);
    Page page() const;
    int selected() const;
    int rows() const;
    int left() const;
    int top() const;
    int submenu_left() const;
    int submenu_top() const;
    const Request& request() const;
    std::optional<int> return_code() const;
    const Settings::State& settings_state() const;
    unsigned confirmation_id() const;
    void saves_present(bool present);
    // Host return values are original callee results, not booleans except inspect_characters.
    s2ne::Result<bool> try_complete_operation(int code = 0);
    void complete_operation(int code = 0);
    int hit(int x, int y) const;
    void move(int delta);
    void cancel();
    void activate();
    Result event(const Settings::Event& e);
    s2ne::Result<std::vector<uint8_t>> try_draw(const std::vector<uint8_t>& battlefield, FullWidthCanvas* canvas = nullptr) const;
    std::vector<uint8_t> draw(const std::vector<uint8_t>& battlefield) const;
};
}    //namespace s2ne
