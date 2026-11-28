#pragma once
#include "combat_visual.h"
#include "disc.h"
#include "events.h"
#include "invisibility.h"
#include "menu.h"
#include "movement_bitmap.h"
#include "palette.h"
#include "skill88_visual.h"
#include "skill_visual.h"
#include <algorithm>
#include <map>
#include <optional>
#include <utility>

namespace s2ne
{
class Map
{
    SharedBytes prg_, pal_;
    std::shared_ptr<const Palette> palette_;
    std::vector<uint8_t> pixels_, terrain_names_;
    unsigned terrain_ground_names_{}, terrain_object_names_{};
    int viewport_extension_{}, viewport_extension_y_{};
    struct Image
    {
        unsigned w, h;
        SharedBytes pixels;
        SharedBytes literal;
        unsigned texture_group{}, texture_id{};
    };
    struct TexturePlacement
    {
        Image image;
        int x{}, y{};
        bool transparent{};
    };
    std::vector<TexturePlacement> texture_placements_;
    mutable std::map<size_t, std::pair<unsigned, unsigned>> texture_items_;
    mutable std::map<size_t, Image> cache_;
    struct UnitBitmap
    {
        unsigned w{}, h{};
        size_t frame{};
        int x{}, y{}, left{}, top{};
        uint8_t state{};
        unsigned status{};
        std::vector<uint8_t> ad, b1;
        bool alternate = true;
    };
    mutable std::map<unsigned, UnitBitmap> unit_bitmaps_;
    mutable std::array<uint32_t, 2> army_modes_{};
    mutable std::array<std::vector<uint8_t>, 2> invisibility_feet_;
    mutable std::vector<uint8_t> invisibility_world_;
    std::array<std::vector<uint8_t>, 2> invisibility_viewports_;
    unsigned invisibility_scratch_{};
    bool invisibility_present_pending_{};
    std::vector<std::vector<uint8_t>> invisibility_presentations_;
    std::vector<std::vector<std::vector<uint8_t>>> invisibility_side_presentations_;
    size_t invisibility_presentation_cursor_{};
    mutable bool write_invisibility_world_{};
    mutable bool read_world_background_{};
    uint8_t viewport_pixel(const std::vector<uint8_t>& source, int x, int y) const;
    std::optional<uint64_t> idle_pose_time_;
    s2ne::SharedEffectClock movement_effect_clock_;
    std::map<unsigned, uint64_t> movement_effect_stamps_;
    struct EffectPresentation
    {
        unsigned actor;
        bool camera;
        uint64_t time;
    };
    mutable std::vector<EffectPresentation> pending_effect_presentations_;
    mutable std::vector<uint8_t> skill88_indexed_;
    s2ne::Disappearance disappearance_;
    std::optional<unsigned> arrival_target_;
    unsigned arrival_frame_{}, arrival_elapsed_{}, arrival_color_{ 194 };
    uint64_t arrival_pose_time_{};
    bool arrival_departure_{};
    bool arrival_presented_{};
    std::optional<unsigned> summon_target_;
    unsigned summon_rows_{}, summon_full_rows_{};
    bool summon_presented_{};
    mutable std::vector<std::vector<uint8_t>> disappearance_frames_;
    mutable uint64_t reaction_draws_{};
    mutable unsigned last_reaction_target_{};
    mutable bool last_reaction_left_{};
    mutable std::vector<uint8_t> last_reaction_;
    mutable std::vector<uint8_t> reaction_foot_mask_;
    mutable std::array<uint8_t, 256> reaction_shadow_{};
    static void overlay_bitmap(UnitBitmap& destination, const UnitBitmap& source, bool background);
    std::filesystem::path root_;
    Disc disc_ = Disc::one;
    bool checker_mask_ = false;
    std::array<uint8_t, 80> event_flags_{};
    std::vector<std::array<unsigned, 4>> dynamic_object_chains_;
    bool composing_static_ = false;
    std::vector<size_t> cleared_object_cells_;
    struct DynamicObject
    {
        int x, y, left, top;
        Image background;
    };
    std::vector<DynamicObject> dynamic_objects_;
    std::vector<std::pair<int, int>> removed_dynamic_cells_;
    unsigned cell_object(size_t cell) const;
    std::vector<unsigned> object_attributes_, scenery_attributes_;
    std::array<std::array<uint8_t, 256>, 2> object_shadows_{};
    size_t group(unsigned id) const;
    Result<size_t> try_item(unsigned archive, unsigned index) const;
    size_t item(unsigned archive, unsigned index) const;
    Result<Image> try_raw(size_t offset) const;
    const Image& raw(size_t offset) const;
    void mask_unit_ground(std::vector<uint8_t>& pixels, unsigned w, unsigned h, int x, int y, int elevation, bool checker = false, int displacement = 0) const;
    Result<Image> try_backdrop(unsigned id) const;
    Image backdrop(unsigned id) const;
    void blit(const Image& image, int x, int y, bool transparent, bool object = false);

public:
    std::vector<std::pair<int, int>> visible_exit_cells() const;
    bool remove_dynamic_object(int x, int y);
    Result<unsigned> try_object_at(int x, int y) const;
    unsigned object_at(int x, int y) const;
    unsigned dynamic_marker(int x, int y) const;
    void mask_selection(std::vector<uint8_t>& pixels, int x, int y) const;
    Result<bool> try_mask_selection_units(std::vector<uint8_t>& pixels, int x, int y, const std::vector<Events::Unit>& units, unsigned turn, uint64_t elapsed, int layer, bool idle_cursor = false) const;
    void set_idle_pose_time(uint64_t time);
    void set_checker_mask(bool enabled);
    void set_dynamic_object_chains(const std::vector<std::array<unsigned, 4>>& chains);
    void set_event_flags(const std::array<uint8_t, 80>& flags);
    unsigned width{}, height{}, map_id{};
    // 42BEED: the terrain area shares the status panel's top/bottom placement.
    Result<bool> draw_terrain_validated(const Menu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const;
    Result<bool> try_draw_terrain(const Menu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const;
    void draw_terrain(const Menu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const;
    void movement_effect_presented();
    unsigned movement_effect_phase() const;
    void set_army_modes(const std::optional<std::array<uint32_t, 2>>& modes) const;
    Result<std::vector<uint8_t>> try_movement_effect(unsigned movement_class, unsigned frame, unsigned direction) const;
    Result<std::vector<uint8_t>> try_movement_shadow(int x, int y) const;
    int camera_x{}, camera_y{};
    // Explicit indexed B1 contract: callers must supply 4483CB's reconstructed
    // background, including 4485DB/448773 roster overlays, never a framebuffer.
    Result<std::vector<uint8_t>> try_compose_reaction(unsigned w, unsigned h, std::span<const uint8_t> current, std::span<const uint8_t> background, bool left) const;
    std::vector<uint8_t> compose_reaction(unsigned w, unsigned h, std::span<const uint8_t> current, std::span<const uint8_t> background, bool left) const;
    void set_resource_domain(Disc disc);
    Map() = default;
    static Result<Map> create(const std::filesystem::path& root, Disc disc = Disc::one);
    explicit Map(const std::filesystem::path& root, Disc disc = Disc::one);
    Result<bool> try_load(unsigned id);
    void load(unsigned id);
    Result<bool> load_validated(unsigned id);
    Result<std::pair<int, int>> try_screen_position(int x, int y) const;
    std::pair<unsigned, unsigned> dimensions() const;
    std::vector<uint8_t> overview(int output_width, int output_height);
    std::pair<int, int> screen_position(int x, int y) const;
    Result<std::pair<int, int>> try_completion_location(unsigned completion) const;
    Result<bool> draw_completion_location(const Menu& font, std::vector<uint8_t>& image, int x, int y) const;
    bool dialogue_visible(int x, int y, unsigned mode = 2) const;
    std::pair<int, int> dialogue_camera(int x, int y) const;
    void movement_camera(int x, int y);
    Result<unsigned> try_idle_frame(unsigned appearance, uint64_t elapsed_ms = 0, unsigned direction = 0, unsigned action = 0) const;
    unsigned idle_frame(unsigned appearance, uint64_t elapsed_ms = 0, unsigned direction = 0, unsigned action = 0) const;
    Result<bool> try_draw_status_figure(std::vector<uint8_t>& image, const Events::Unit& unit, int center, int bottom) const;
    unsigned unit_height(unsigned appearance, unsigned direction = 0, unsigned action = 0) const;
    // Renderer identities are indices in the full roster, never resource/story Unit::id values.
    std::vector<uint8_t> expected_reaction_bitmap(unsigned id, bool left) const;
    const std::vector<uint8_t>& unit_background_bitmap(unsigned id) const;
    const std::vector<uint8_t>& unit_source_bitmap(unsigned id) const;
    std::array<int, 4> unit_bitmap_bounds(unsigned id) const;
    bool unit_bitmap_available(unsigned id) const;
    bool begin_teleport_arrival(const std::vector<Events::Unit>& roster, unsigned target, unsigned turn = 1, bool departure = false, uint64_t elapsed = 0, unsigned color = 0);
    bool teleport_arrival_active() const;
    Result<bool> begin_summon_growth(const std::vector<Events::Unit>& roster, unsigned target, unsigned turn = 1);
    bool summon_growth_active() const;
    std::optional<unsigned> summon_growth_target() const;
    unsigned summon_growth_rows() const;
    void summon_growth_presented();
    void tick_summon_growth();
    void cancel_summon_growth();
    unsigned teleport_arrival_frame() const;
    void teleport_arrival_presented();
    void tick_teleport_arrival(unsigned ms);
    void cancel_teleport_arrival();
    bool begin_disappearance(const std::vector<Events::Unit>& roster, unsigned target, unsigned turn = 1, uint64_t elapsed = 0, bool death = false, std::string* call_error = nullptr);
    void enable_disappearance_presentation();
    void disappearance_presented();
    void tick_disappearance(unsigned ms);
    bool disappearance_active() const;
    bool disappearance_finished() const;
    unsigned disappearance_frame() const;
    const std::vector<uint8_t>& disappearance_pixels() const;
    unsigned last_reaction_target() const;
    bool last_reaction_left() const;
    uint64_t reaction_draws() const;
    const std::vector<uint8_t>& last_reaction_bitmap() const;
    // Full roster order is required for 4485DB's persistent B1 overlays.
    Result<bool> draw_units_validated(std::vector<uint8_t>& image, const std::vector<Events::Unit>& units, unsigned turn = 1, uint64_t elapsed_ms = 0, std::optional<unsigned> silhouette = std::nullopt, std::optional<unsigned> reaction = std::nullopt, unsigned background_mode = 0, std::optional<unsigned> only_instance = std::nullopt, std::vector<uint8_t>* indexed = nullptr) const;
    Result<bool> try_draw_units(std::vector<uint8_t>& image, const std::vector<Events::Unit>& units, unsigned turn = 1, uint64_t elapsed_ms = 0, std::optional<unsigned> silhouette = std::nullopt, std::optional<unsigned> reaction = std::nullopt, unsigned background_mode = 0, std::optional<unsigned> only_instance = std::nullopt, std::vector<uint8_t>* indexed = nullptr) const;
    void draw_units(std::vector<uint8_t>& image, const std::vector<Events::Unit>& units, unsigned turn = 1, uint64_t elapsed_ms = 0, std::optional<unsigned> silhouette = std::nullopt, std::optional<unsigned> reaction = std::nullopt, unsigned background_mode = 0, std::optional<unsigned> only_instance = std::nullopt, std::vector<uint8_t>* indexed = nullptr) const;
    Result<bool> prepare_invisibility_buffers(const std::vector<Events::Unit>& baseline,
        const std::vector<Events::Unit>& staged, const std::vector<size_t>& targets, unsigned turn, uint64_t elapsed, bool wide = true);
    const std::vector<uint8_t>& invisibility_world() const;
    Result<bool> refresh_world(const std::vector<Events::Unit>& roster, unsigned turn, uint64_t elapsed);

private:
    Result<bool> refresh_world_candidate(const std::vector<Events::Unit>& roster, unsigned turn, uint64_t elapsed, const std::vector<uint8_t>* background = nullptr);

public:
    Result<std::vector<uint8_t>> world_viewport() const;
    Result<bool> try_draw_world(std::vector<uint8_t>& image, const std::vector<Events::Unit>& roster, unsigned turn, uint64_t elapsed, bool contains_background = false) const;
    Result<bool> refresh_army_world(const std::vector<Events::Unit>& roster, bool hostile, unsigned turn, uint64_t elapsed);
    const std::vector<uint8_t>& invisibility_viewport(unsigned index) const;
    unsigned invisibility_scratch() const;
    bool invisibility_present_pending() const;
    void invisibility_presented();
    std::vector<uint8_t> present_invisibility_viewport(std::optional<unsigned> side = std::nullopt) const;
    Result<std::vector<uint8_t>> skill32_background(const std::vector<Events::Unit>& units, unsigned turn, unsigned caster, unsigned target, unsigned tint_source = 140, unsigned tint_amount = 45) const;
    std::vector<uint8_t> skill32_frame(const std::vector<uint8_t>& background, const Skill32Visual::Frame& frame, const std::vector<Events::Unit>& units, unsigned turn, unsigned target, const Skill32Visual* visual = nullptr) const;
    Result<std::vector<uint8_t>> skill88_background(const std::vector<Events::Unit>& units, unsigned turn, unsigned caster, unsigned tint_amount = 50, unsigned tint_source = 233, std::optional<unsigned> partner = std::nullopt) const;
    std::vector<uint8_t> skill88_frame(const std::vector<uint8_t>& background, const Skill88Visual& visual, const Events::Unit& actor, std::optional<size_t> frame_index = std::nullopt, const std::vector<Events::Unit>* occluders = nullptr, unsigned turn = 1) const;
    std::pair<int, int> constrained_camera(int x, int y) const;
    void constrain_camera();
    void set_viewport_extension(int extension, int vertical = 0);
    void pan(int dx, int dy);
    bool edge_scroll(float x, float y);
    bool evacuation_visible(int x, int y) const;
    bool evacuation_camera_step(int x, int y);
    void center(int x, int y);
    std::vector<uint8_t> draw() const;
    Result<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> sight_background(const std::vector<Events::Unit>& units, unsigned turn, uint64_t elapsed) const;
    // 42FE12 supplies random object-name entries to 46243A and explicitly
    // skips type12; it is not a water-animation timer. Palette cycling is
    // applied by the presentation layer, not by changing map object IDs.
    std::vector<uint8_t> draw(uint64_t elapsed_ms) const;
};
}    //namespace s2ne
