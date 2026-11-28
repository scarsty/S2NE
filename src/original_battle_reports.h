#pragma once
#include "original_map.h"
#include "original_settings.h"
#include "original_skills.h"
#include "original_sprite.h"
#include "original_unit_status.h"
#include <functional>
#include <memory>

namespace s2ne
{
class OriginalBattleReports
{
public:
    enum class Page
    {
        conditions,
        roster,
        map,
        detail,
        abilities,
        proficiencies,
        status,
        equipment,
        closed
    };
    struct Entry
    {
        OriginalEvents::Unit unit;
        bool enemy = false;
        int runtime_kind = 0;
        bool locate_blocked = false;
    };
    struct Result
    {
        bool changed = false;
        unsigned consumed_mask = 0;
    };
    static constexpr unsigned conditions_address = 0x43260D, roster_address = 0x43471C, map_address = 0x4352CE;

private:
    OriginalMenu font_;
    OriginalUnitStatus status_;
    OriginalSkills skills_;
    OriginalMap unit_resources_;
    SharedBytes cel_, pal_, items_;
    mutable OriginalPalette palette_;
    std::vector<int> combat_values_;
    bool enemy_strength_visible_ = true;
    std::vector<std::vector<uint8_t>> ability_names_;
    std::vector<uint8_t> background_, mini_;
    std::vector<Entry> entries_;
    std::vector<uint8_t> stage_name_, victory_, defeat_;
    unsigned stage_ = 0;
    Page page_;
    bool close_to_battle_ = false;
    int first_ = 0, row_ = 0, code_ = 0, mini_w_ = 0, mini_h_ = 0;
    std::optional<std::pair<int, int>> locate_;
    unsigned detail_address_ = 0;
    int friendly_count_ = 0, npc_count_ = 0, enemy_count_ = 0;
    unsigned detail_button_ = 0;
    std::vector<std::array<bool, 2>> inspection_available_;
    bool detail_button_available(unsigned button) const;
    static s2ne::Result<SharedBytes> try_read(const std::filesystem::path& p);
    static std::vector<uint8_t> bytes(const std::string& s);
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned c) const;
    s2ne::Result<bool> try_sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, bool transparent = true) const;
    void sprite(std::vector<uint8_t>& image, unsigned id, int left, int top, bool transparent = true) const;
    s2ne::Result<bool> large(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color) const;
    s2ne::Result<bool> small(std::vector<uint8_t>& image, const std::vector<uint8_t>& text, int x, int y, unsigned color) const;
    s2ne::Result<bool> try_detail(std::vector<uint8_t>& image, FullWidthCanvas* canvas, uint64_t elapsed) const;
    s2ne::Result<bool> try_roster_row_validated(std::vector<uint8_t>& image, int row, const Entry& entry) const;
    s2ne::Result<bool> try_roster_row(std::vector<uint8_t>& image, int row, const Entry& entry) const;
    void roster_row(std::vector<uint8_t>& image, int row, const Entry& entry) const;
    s2ne::Result<bool> try_build_mini(OriginalMap map);

private:
    // Entries must retain the original two army arrays' order; no hostility reclassification.
    OriginalBattleReports(OriginalMenu font, OriginalUnitStatus status, SharedBytes cel, SharedBytes pal, Page page, const OriginalEvents& events,
        const std::vector<uint8_t>& background);
    s2ne::Result<bool> initialize(const std::filesystem::path& root, Page page, const OriginalEvents& events,
        std::vector<Entry> entries, const std::vector<uint8_t>& background, const OriginalMap* map, bool small_font, int dword_4f7cd0);

public:
    OriginalBattleReports(const std::filesystem::path& root, Page page, const OriginalEvents& events,
        std::vector<Entry> entries, const std::vector<uint8_t>& background, const OriginalMap* map = nullptr, bool small_font = false, int dword_4f7cd0 = 0);
    static s2ne::Result<std::unique_ptr<OriginalBattleReports>> create(const std::filesystem::path& root, Page page, const OriginalEvents& events,
        std::vector<Entry> entries, const std::vector<uint8_t>& background, const OriginalMap* map = nullptr, bool small_font = false, int dword_4f7cd0 = 0);

    Page page() const;
    int first() const;
    int selected_row() const;
    size_t size() const;
    int return_code() const;
    std::optional<std::pair<int, int>> location() const;
    const Entry& selected() const;
    unsigned detail_address() const;
    void close_to_battle(bool enabled = true);
    void close_detail();
    void proficiencies();
    void abilities();
    void status_page();
    void equipment_page();
    void detail();
    void locate();
    void navigate(unsigned scan);
    int hit(int x, int y) const;
    Result event(const OriginalSettings::Event& e);
    s2ne::Result<std::vector<uint8_t>> try_draw(FullWidthCanvas* canvas = nullptr) const;
    std::vector<uint8_t> draw(FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
