#pragma once
#include "original_events.h"
#include "original_menu.h"
#include "original_sprite.h"
#include <memory>
#include <string>

namespace s2ne
{
// 4571EB draws over the caller's original panel background; panel_y is 0 or 363.
class OriginalUnitStatus
{
    SharedBytes cel_, pal_;
    size_t heads_{};
    mutable std::vector<uint8_t> cached_input_, cached_output_;
    mutable std::vector<uint8_t> cached_name_, cached_profession_;
    mutable std::array<int64_t, 26> cached_fields_{};
    mutable const OriginalMenu* cached_font_{};
    void pixel(std::vector<uint8_t>& image, int x, int y, unsigned c) const;
    void rectangle(std::vector<uint8_t>& image, int x, int y, int right, int bottom, unsigned c) const;
    s2ne::Result<bool> try_portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const;

    void portrait(std::vector<uint8_t>& image, unsigned id, int x, int y) const;
    static std::vector<uint8_t> bytes(const std::string& s);

    OriginalUnitStatus(SharedBytes cel, SharedBytes pal);
    s2ne::Result<bool> initialize();

public:
    explicit OriginalUnitStatus(const std::filesystem::path& root);
    static s2ne::Result<std::unique_ptr<OriginalUnitStatus>> create(const std::filesystem::path& root);

    // Returns the missing right-hand segment, exactly as IDIV in 4573A1 (no ratio clamp).
    static int missing_bar(int current, int limit);
    s2ne::Result<bool> try_draw_status_chain(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u,
        int x_base, int y_base) const;
    s2ne::Result<bool> try_draw_report(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u,
        int x_base, int y_base, bool alternate_context = false) const;
    s2ne::Result<bool> try_draw(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u, int panel_y,
        bool alternate_context = false) const;
    void draw(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& u, int panel_y, bool alternate_context = false) const;
};
}    //namespace s2ne
