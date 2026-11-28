#pragma once
#include "battle.h"
#include "original_battle_captions.h"
#include "original_campaign.h"
#include "original_confirmation.h"
#include "original_death_dispatch.h"
#include "original_dialogue_layout.h"
#include "original_ending.h"
#include "original_evacuation.h"
#include "original_events.h"
#include "original_idle_clock.h"
#include "original_interbattle.h"
#include "original_map.h"
#include "original_menu.h"
#include "original_palette.h"
#include "original_progression.h"
#include "original_script_predicates.h"
#include "original_scripted_movement.h"
#include "original_scroll_text.h"
#include "original_skill94_visual.h"
#include "original_skills.h"
#include "original_sprite.h"
#include "original_ui_timing.h"
#include "original_unit_status.h"
#include "original_worldmap.h"
#include "wide_viewport.h"
#include <deque>
#include <map>
#include <optional>

namespace s2ne
{
class Prologue
{
    SharedBytes cel_;
    std::vector<uint8_t> pal_, caption_items_;
    mutable std::optional<OriginalPalette> palette_effects_;
    OriginalUnitStatus unit_status_;
    mutable OriginalMap map_;
    mutable std::vector<uint8_t> battle_mini_;
    mutable unsigned battle_mini_map_{};
    mutable std::vector<std::array<unsigned, 4>> battle_mini_chains_;
    std::optional<OriginalSkill32Visual> skill32_visual_;
    std::string skill32_visual_error_;
    std::optional<OriginalSkill88Visual> skill88_visual_;
    std::vector<unsigned> skill78_targets_;
    size_t skill78_frame_count_{};
    struct SpellCell
    {
        OriginalEvents::Unit anchor;
        size_t start;
    };
    std::vector<SpellCell> spell_cells_;
    std::optional<OriginalSkill88Visual> spell_followup_;
    size_t spell_primary_frames_{};
    unsigned spell_dispatch_{};
    std::optional<OriginalSkill94Visual> skill94_visual_;
    std::vector<uint8_t> skill32_background_;
    mutable std::vector<uint8_t> spell_shake_frame_;
    std::vector<uint8_t> spell_shake_source_;
    mutable int spell_shake_cursor_{ -1 };
    struct VisualSlice
    {
        OriginalMap map;
        std::vector<uint8_t> background, shake_source;
        mutable std::vector<uint8_t> shake_frame;
        mutable int shake_cursor{ -1 };
    };
    std::vector<VisualSlice> visual_slices_;
    mutable std::vector<uint8_t> wide_shake_frame_;
    mutable int wide_shake_cursor_{ -1 };
    unsigned skill32_target_{};
    std::optional<Battle::Point> skill_area_center_;
    bool battle_menu_overlay_{};
    std::function<void()> skill94_death_commit_;
    bool skill94_death_presented_{};
    unsigned skill94_death_frame_{};
    struct Teleport
    {
        unsigned target{}, phase{};
        Battle::Point destination;
        uint8_t action{};
        bool presented{};
        uint64_t pose_time{};
    };
    std::optional<Teleport> teleport_;
    std::optional<OriginalEvents> teleport_baseline_;
    std::optional<Battle> teleport_battle_baseline_;
    std::optional<OriginalMap> teleport_map_baseline_;
    std::pair<int, int> teleport_camera_;
    std::function<void()> spell_transfer_publish_;
    std::optional<unsigned> spell_transfer_visual_source_;
    std::vector<OriginalEvents::Unit> spell_transfer_roster_;
    bool spell_transfer_final_failure_for_test_{};
    std::vector<OriginalEvents::Unit> summon_roster_;
    std::function<void()> summon_publish_;
    std::optional<OriginalMap> summon_map_;
    OriginalEvacuation evacuation_;
    bool evacuation_presented_{};
    std::optional<std::pair<int, int>> death_location_;
    unsigned death_location_ms_{};
    bool death_location_presented_{};
    bool evacuation_camera_done_{};
    unsigned evacuation_visible_steps_{};
    std::optional<std::pair<int, int>> death_camera_baseline_;
    std::optional<OriginalDeathDispatch::Stack> synchronous_death_stack_;
    std::optional<OriginalEvents> synchronous_death_baseline_;
    std::optional<OriginalEvents> damage_area_baseline_;
    std::vector<unsigned> damage_area_hidden_;
    std::vector<OriginalEvents::Unit> damage_area_poses() const;
    std::pair<int, int> damage_area_camera_;
    std::function<void()> synchronous_death_return_;
    std::filesystem::path root_;
    OriginalDisc disc_{ OriginalDisc::one };
    std::vector<OriginalSkill32Visual::Frame> defeat_frames_;
    size_t defeat_frame_{};
    unsigned defeat_clock_{};
    unsigned defeat_interval_{};
    mutable bool defeat_presented_{};
    std::optional<Battle> battle_;
    std::function<Result<bool>(const std::vector<OriginalEvents::Unit>&, const std::vector<Battle::PhysicalSettlement>&)> physical_loot_commit_;
    std::optional<OriginalEvents> physical_death_baseline_;
    std::function<Result<bool>(bool)> physical_loot_finish_;
    std::vector<Battle::PhysicalSettlement> growth_awards_;
    size_t growth_award_cursor_{};
    std::vector<OriginalEvents::Unit> growth_units_;
    std::optional<OriginalProgression::PendingGrowth> growth_;
    OriginalRandom::State growth_random_;
    uint32_t growth_seconds_{};
    std::function<void(bool)> growth_finish_;
    bool growth_presented_{};
    unsigned growth_elapsed_{};
    bool reward_growth_external_{};
    void advance_growth();
    std::optional<std::pair<int, int>> physical_death_camera_;
    std::optional<size_t> physical_departure_;
    bool physical_departure_presented_{};
    unsigned physical_departure_notice_ms_{};
    void finish_physical_death(size_t target, bool departure);
    void connect_combat_progression();
    std::optional<OriginalConfirmation> finish_confirmation_;
    std::optional<unsigned> finish_direction_;
    enum class Phase
    {
        dialogue,
        choice,
        branch,
        flag_dialogue,
        briefing,
        battle_intro,
        objectives,
        battle,
        hostile_actions,
        automatic_actions,
        npc_actions,
        story_animation,
        story_movement,
        story_disappearance,
        story_wait,
        story_ready,
        campaign_dialogue,
        campaign_rewards,
        campaign_defeat,
        interbattle,
        campaign_blocked
    };
    Phase synchronous_death_phase_ = Phase::battle;
    mutable std::optional<OriginalCampaign> campaign_;
    std::optional<OriginalInterbattle> interbattle_;
    bool deployment_requested_{};
    bool deployment_completed_{};
    std::deque<size_t> script_queue_;
    unsigned arrivals_turn_{};

public:
    struct ArrivalTransaction
    {
        OriginalEvents events;
        // Must be noexcept: all fallible resource preparation precedes publication.
        std::function<void()> commit;
        std::function<void()> rollback;
    };

private:
    std::function<std::optional<ArrivalTransaction>(const OriginalEvents&, unsigned)> campaign_arrival_adapter_;
    std::function<std::optional<ArrivalTransaction>(const OriginalEvents&, size_t)> campaign_scripted_roles_adapter_;
    Phase script_return_{ Phase::battle };
    std::optional<OriginalCampaign::Decision> campaign_decision_;
    std::vector<OriginalCampaign::SaveRequest> autosave_requests_;
    std::string campaign_blocker_;
    std::function<bool(OriginalEvents&, const OriginalCampaign::Settlement&)> campaign_reward_adapter_;
    std::function<OriginalInterbattle::Result(OriginalInterbattle::Request)> interbattle_adapter_;
    bool campaign_settled_{};
    OriginalCampaign& campaign() const;
    void sync_event_flags();
    unsigned intro_elapsed_ = 0;
    bool presentation_driven_ = false;
    OriginalBannerPresentation banner_presentation_;
    unsigned animation_elapsed_ = 0;
    OriginalIdleClock idle_clock_;
    std::optional<std::pair<int, int>> dialogue_camera_target_;
    std::pair<int, int> dialogue_camera_subject_{};
    unsigned dialogue_visible_steps_ = 0;
    bool first_banner_ = true;
    mutable int status_panel_side_ = 1;
    // Nine original poses, including the settled pose, must reach the renderer.
    // 4389A9 passes edx=1 to 473111: one centisecond minimum per presented pose.
    static constexpr unsigned banner_pose_duration_ = 10;
    static constexpr unsigned banner_duration_ = 9 * banner_pose_duration_;
    struct Banner
    {
        int y = 0, turn_x = 0, number_x = 620, strip = 1;
        bool done = false;
    };
    static Banner banner(unsigned frame);
    Phase phase_ = Phase::dialogue;
    unsigned chosen_ = 0;
    unsigned story_route_{};
    struct StoryPose
    {
        size_t unit{};
        unsigned frames{}, action{};
        uint8_t action_state{};
    };
    std::vector<StoryPose> story_poses_;
    unsigned story_pose_elapsed_{};
    unsigned story_wait_ms_{};
    size_t story_wait_event_{};
    bool story_turning_{};
    std::optional<size_t> story_animation_event_;
    Phase story_movement_return_{ Phase::story_ready };
    std::optional<s2ne::Group> story_group_;
    std::vector<size_t> story_group_units_;
    std::vector<size_t> story_hidden_units_;
    std::optional<s2ne::Track> story_movement_;
    size_t story_movement_event_{}, story_movement_unit_{};
    bool story_movement_follow_{};
    size_t story_disappearance_event_{}, story_disappearance_unit_{};
    unsigned story_disappearance_slot_{};
    std::optional<size_t> death_presentation_event_;
    std::vector<std::function<void()>> death_presentation_commits_;
    std::optional<Battle> death_battle_baseline_;
    std::optional<OriginalMap> death_map_baseline_;
    bool death_presentation_presented_{};
    unsigned death_camera_wait_ms_{};
    void next_story_disappearance();
    bool begin_story_animation(size_t event);
    std::optional<OriginalEnding> ending_;
    bool campaign_choice_{};
    bool restored_story_pending_{};
    std::optional<Phase> scripted_battle_return_;
    Phase campaign_choice_return_{ Phase::battle };
    bool story_stage() const;
    void dispatch_story(bool starting_battle_script = false);
    int selection_ = 0;
    std::optional<OriginalConfirmation> choice_confirmation_;
    std::vector<uint8_t> briefing_page_;
    std::optional<OriginalWorldmap> travel_map_;
    size_t briefing_offset_ = 0;
    size_t briefing_visible_{};
    unsigned briefing_elapsed_{};
    bool briefing_presented_{};
    size_t event_ = 0, line_ = 0, page_ = 0;
    struct Glyph
    {
        std::vector<uint8_t> bytes;
        unsigned color;
        int x{}, y{};
        unsigned delay{};
        bool default_delay = true;
    };
    std::vector<std::vector<Glyph>> pages_;
    bool text_animation_ = false, small_font_ = false;
    unsigned message_delay_ = 10, text_elapsed_ = 0;
    size_t revealed_glyphs_ = 0;
    size_t group(unsigned id) const;
    Result<size_t> try_sprite_offset(unsigned archive, unsigned index) const;
    size_t sprite_offset(unsigned archive, unsigned index) const;
    Result<bool> sprite_validated(std::vector<uint8_t>& image, size_t p, bool typed, int left, int top, bool opaque_zero = false) const;
    Result<bool> try_sprite(std::vector<uint8_t>& image, size_t p, bool typed, int left, int top, bool opaque_zero = false) const;
    void sprite(std::vector<uint8_t>& image, size_t p, bool typed, int left, int top, bool opaque_zero = false) const;
    Result<bool> try_prepare();
    void rollback_synchronous_death();
    bool prepare();
    void enter_stage_battle();
    void prepare_briefing();
    bool start_campaign_script();
    bool prepare_campaign_transaction_pose();
    bool role_removal_pending_{};
    bool begin_role_removal(size_t event);
    bool apply_scripted_control(size_t event);
    bool dispatch_original_flag_operations();
    bool begin_scripted_party_experience(size_t event, size_t cursor = 0);
    bool dispatch_campaign();

public:
    OriginalEvents events;
    bool interbattle_random(const std::function<bool(OriginalRandom&, uint32_t)>& transaction);
    Battle& battle();
    const Battle& battle() const;
    void reconnect_battle_callbacks();
    void queue_original_death(std::vector<OriginalEvents::Unit>& units, size_t actor, size_t target);
    bool ending_active() const;
    std::function<void(unsigned)> skill_effect_audio_;
    std::vector<uint8_t> skill_effect_sounds_;
    std::vector<uint8_t> skill_followup_sounds_;
    OriginalActionAudioCues skill_audio_cues_;
    std::function<Result<bool>()> stop_effect_audio_;
    std::function<Result<unsigned>(unsigned, unsigned)> script_effect_audio_;
    OriginalAniAudio::Music music_audio_;
    OriginalAniAudio::Music midi_audio_;
    unsigned music_restore_generation_{};
    std::optional<OriginalScrollText> temporary_text_;
    size_t temporary_event_{};
    unsigned current_script_music_{}, temporary_restore_music_{};
    int dialogue_music_after_{};
    Result<bool> begin_temporary_text(unsigned background, unsigned text, unsigned music, size_t event);
    bool dispatch_sound_event(size_t i);
    void set_script_effect_audio(OriginalAniAudio::Play play, OriginalAniAudio::Stop stop);
    void set_music_audio(OriginalAniAudio::Music play, OriginalAniAudio::Music midi = {});
    ResourceDomain music_domain() const;
    unsigned music_restore_generation() const;
    bool has_battle() const;
    Result<bool> play_death_effect();
    void play_skill_effect(unsigned skill);
    void set_skill_effect_audio(std::function<void(unsigned)> play);
    void set_ending_effect_audio(OriginalAniAudio::Play play, OriginalAniAudio::Stop stop);
    bool ending_done() const;
    size_t ending_page() const;
    size_t ending_pages() const;
    std::string ending_description() const;
    void restore_terminal_ending();
    bool campaign_modal() const;
    bool campaign_dialogue() const;
    bool campaign_defeated() const;
    bool defeat_finished() const;
    bool campaign_reward_report() const;
    unsigned last_reaction_target() const;
    bool last_reaction_left() const;
    uint64_t reaction_draws() const;
    std::vector<std::pair<int, int>> visible_exit_cells() const;
    const std::vector<uint8_t>& last_reaction_bitmap() const;
    std::vector<uint8_t> expected_reaction_bitmap(unsigned id, bool left) const;
    bool interbattle_active() const;
    bool interbattle_services_allowed() const;
    const std::string& campaign_blocker() const;
    void set_campaign_blocker(std::string message);
    const std::vector<s2ne::OriginalCampaign::SaveRequest>& autosave_requests() const;
    bool campaign_travel_pending() const;
    bool campaign_travel_active() const;
    void begin_campaign_travel(OriginalWorldmap map);
    void clear_autosave_requests();
    void set_campaign_reward_adapter(std::function<bool(OriginalEvents&, const OriginalCampaign::Settlement&)> callback);
    void set_physical_loot_finish(std::function<Result<bool>(bool)> callback);
    bool growth_active() const;
    bool begin_reward_growth(OriginalProgression::PendingGrowth pending, uint32_t seconds, std::function<void(bool)> finish);
    const OriginalProgression::PendingGrowth& reward_growth() const;
    int experience_notice() const;
    Result<bool> draw_experience_notice(const OriginalMenu& font, std::vector<uint8_t>& image) const;
    Result<bool> draw_pickup_notice(const OriginalMenu& font, std::vector<uint8_t>& image, size_t unit_index, unsigned item_id) const;
    const std::vector<uint8_t>* learning_notice() const;
    void reject_growth();
    const OriginalProgression::GrowthStep* growth_step() const;
    void confirm_growth();
    void set_physical_loot_commit(std::function<Result<bool>(const std::vector<OriginalEvents::Unit>&, const std::vector<Battle::PhysicalSettlement>&)> callback);
    void set_campaign_arrival_adapter(std::function<std::optional<ArrivalTransaction>(const OriginalEvents&, unsigned)> callback, unsigned already_inserted_turn = 0);
    void mark_campaign_arrivals_inserted(unsigned turn);
    Result<bool> try_normalize_campaign_roster(OriginalEvents restored, unsigned already_inserted_turn = 1);
    void normalize_campaign_roster(OriginalEvents restored, unsigned already_inserted_turn = 1);
    void set_campaign_scripted_roles_adapter(std::function<std::optional<ArrivalTransaction>(const OriginalEvents&, size_t)> callback);
    void set_resource_domain(OriginalDisc disc);
    void set_interbattle_adapter(std::function<OriginalInterbattle::Result(OriginalInterbattle::Request)> callback);
    bool select_interbattle_option(unsigned index);
    void campaign_scan(unsigned scan);
    std::optional<OriginalInterbattle::Request> interbattle_request() const;
    unsigned pending_story_route() const;
    unsigned campaign_next_stage() const;
    void restore_campaign_entry(OriginalEvents restored);
    Result<bool> try_restore_campaign_story(OriginalEvents restored);
    void restore_campaign_story(OriginalEvents restored);
    Result<bool> try_resume_campaign_story_route(OriginalEvents restored);
    void resume_campaign_story_route(OriginalEvents restored);
    Result<bool> try_resume_campaign_stage(OriginalEvents restored);
    void resume_campaign_stage(OriginalEvents restored);
    bool finished = false;
    struct InitializationTag
    {
    };
    Prologue(const std::filesystem::path& root, unsigned difficulty, InitializationTag);
    Result<bool> initialize(const std::filesystem::path& root);
    static Result<std::unique_ptr<Prologue>> create(const std::filesystem::path& root, unsigned difficulty);
    explicit Prologue(const std::filesystem::path& root, unsigned difficulty);
    bool choosing() const;
    bool in_battle() const;
    uint64_t idle_pose_time() const;
    bool palette_cycle_due() const;
    bool hostile_actions() const;
    bool automatic_actions() const;
    bool npc_actions() const;
    bool computer_actions() const;
    const s2ne::Feedback* combat_feedback() const;
    bool battle_intro() const;
    void enable_presentation_timing();
    bool teleport_active() const;
    bool inject_spell_transfer_final_failure_for_test();
    bool spell_summon_active() const;
    bool inject_spell_summon_failure_for_test();
    Result<bool> begin_spell_summon(std::vector<OriginalEvents::Unit> roster, unsigned target, std::function<void()> publish);
    void set_spell_transfer_visual_source(unsigned target);
    bool inject_spell_transfer_failure_for_test();
    void rollback_teleport();
    bool begin_spell_transfer(unsigned target, Battle::Point destination, std::function<void()> publish);
    void configure_teleport();
    bool evacuation_active() const;
    Result<bool> begin_death_presentation(OriginalEvents& source, size_t event, bool dryRun);
    void continue_synchronous_death();
    bool synchronous_death_active() const;
    bool inject_synchronous_death_failure_for_test();
    bool spirit_sight_pending = false;
    bool invisibility_presentation_pending() const;
    bool original_save_boundary_ready() const;
    bool temporary_text_active() const;
    void observe_music(unsigned id);
    void cancel_temporary_text();
    bool deployment_requested() const;
    void commit_deployment(OriginalEvents events);
    bool synchronous_death_supported(unsigned role);
    bool begin_synchronous_death(unsigned role, std::function<void()> resume, const OriginalEvents::Unit* dying = nullptr);
    bool begin_skill94_death(unsigned target, std::function<void()> commit);
    struct DamagePresentation
    {
        unsigned target{};
        int hp{}, damage{}, healing{}, snapshot{};
        bool killed{}, revived{}, reverse{};
        OriginalEvents::Unit dying;
        unsigned effect{ 1 };
        int caster_hp{}, caster_max_hp{};
        bool reversed_damage{};
        bool hit_flash{};
        bool direct_death{};
    };
    bool begin_damage_area_feedback(unsigned actor, std::shared_ptr<std::vector<DamagePresentation>> targets,
        size_t cursor, std::function<void()> publish);
    void frame_presented();
    struct ItemPresentation
    {
        size_t actor{}, target{};
        unsigned frames{}, elapsed{}, step{};
        bool presented{}, tint{};
        OriginalEvents::Unit before;
        std::function<void()> commit;
    };
    std::optional<ItemPresentation> item_presentation_;
    Result<bool> begin_item_presentation(size_t actor, size_t target, bool tint, std::function<void()> commit);
    bool skill_visual_active() const;
    std::optional<Battle::Point> skill_camera_target_;
    bool prepare_skill_camera(int x, int y);
    Result<OriginalMap> prepare_invisibility_scratch(const std::vector<OriginalEvents::Unit>& staged, const std::vector<size_t>& targets);
    void commit_invisibility_scratch(OriginalMap prepared);
    Result<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> prepare_spirit_sight_background() const;
    Result<OriginalMap> prepare_army_reveal_world(bool caster_hostile, bool reject_bitmap_for_test = false);
    bool begin_skill88_visual(unsigned caster, bool healing1 = false, bool sleep2 = false, std::optional<Battle::Point> center = std::nullopt, bool blessing3 = false, unsigned statusSkill = 0, std::optional<unsigned> performer = std::nullopt, std::optional<unsigned> partner = std::nullopt, std::vector<unsigned> targets = {}, std::string* call_error = nullptr);
    int skill94_visual_frame() const;
    bool begin_skill94_visual(unsigned caster, unsigned target);
    bool skill32_visual_active() const;
    const std::string& skill32_visual_error() const;
    int skill32_visual_frame() const;
    int skill88_visual_frame() const;
    bool begin_skill32_visual(unsigned caster, unsigned target, bool curse33 = false, std::string* call_error = nullptr);
    bool showing_objectives() const;
    int diagnostic_phase() const;
    size_t diagnostic_event() const;
    size_t diagnostic_line() const;
    void set_small_font(bool enabled);
    void enable_text_animation(unsigned delay);
    bool text_revealing() const;
    size_t revealed_glyphs() const;
    void reveal_text();
    void tick(unsigned milliseconds);
    Result<bool> try_tick(unsigned milliseconds);
    int selection() const;
    bool choice_confirmation_pending() const;
    void cancel_choice_confirmation();
    void move(int delta);
    void pan(int dx, int dy);
    void edge_scroll(float x, float y);
    void set_auto_selection(bool enabled);
    void select_ordered_player(bool restart);
    void set_mask_surfaces(bool enabled);
    void set_checker_mask(bool enabled);
    void set_battle_menu_overlay(bool enabled);
    void sync_movement_contexts();
    static std::vector<Battle::MovementContext> movement_contexts(const OriginalEvents& events, const OriginalMap& map);
    void mouse(float x, float y, bool click);
    void begin_finish_confirmation();
    void resolve_finish();
    void finish_facing(unsigned direction);
    void cancel_battle_selection();
    void battle_confirm();
    void battle_cycle(int delta);
    void battle_cycle_target();
    Battle::State battle_state() const;
    std::pair<int, int> camera() const;
    void restore_battle(OriginalEvents restored, unsigned turn, bool resume_story = true, bool interbattle = false);
    void show_objectives();
    bool drawing_original_mouse() const;
    Battle::Point battle_command_origin() const;
    void start_auto_battle();
    void end_battle_turn();
    bool hostile_banner() const;
    std::string battle_status() const;
    int battle_selected() const;
    int battle_command_index() const;
    int battle_requested_command() const;
    void clear_battle_requested_command();
    unsigned item_marker(int x, int y) const;
    bool secret_item_at(int x, int y) const;
    void draw_secret_markers(std::vector<uint8_t>& image) const;
    unsigned pickup_at_unit(size_t index, std::vector<uint8_t>& inventory, uint8_t army_metadata = 0);
    unsigned battle_turn() const;
    Battle::Point screen_cell(Battle::Point p) const;
    std::optional<OriginalDialogueLayout> dialogue_layout() const;
    size_t line_index() const;
    size_t page_index() const;
    bool dialogue_scrolling() const;
    void advance();
    Result<bool> try_advance();
    void draw_status_terrain(const OriginalMenu& font, std::vector<uint8_t>& image, const OriginalEvents::Unit& unit, int panel_y) const;
    void draw_terrain_panel(const OriginalMenu& font, std::vector<uint8_t>& image, int x, int y, int panel_y) const;
    void draw_battle_edge_hud(const OriginalMenu& font, std::vector<uint8_t>& image) const;
    void mask_ground_selection(std::vector<uint8_t>& pixels, Battle::Point cell) const;
    std::vector<uint8_t> draw(const OriginalMenu& font, const std::function<void(std::vector<uint8_t>&)>& groundOverlay = {}, bool commandOverlay = true) const;
    void cycle_frame(std::vector<uint8_t>& image) const;
    Result<std::vector<uint8_t>> prepare_visual_background(const std::function<Result<std::vector<uint8_t>>(OriginalMap&)>& prepare, bool shake = false);
    std::vector<uint8_t> wide_frame(const OriginalMenu& font, const std::vector<uint8_t>& center, bool cycle = true, const std::function<void(std::vector<uint8_t>&)>& groundOverlay = {}) const;
    std::vector<uint8_t> draw_visual(const OriginalMap& map_, const std::vector<uint8_t>& skill32_background_, const std::vector<uint8_t>& spell_shake_source_, std::vector<uint8_t>& spell_shake_frame_, int& spell_shake_cursor_) const;
    std::vector<uint8_t> draw_uncycled(const OriginalMenu& font, const std::function<void(std::vector<uint8_t>&)>& groundOverlay, bool commandOverlay = true, bool worldOnly = false, FullWidthCanvas* canvas = nullptr) const;
};
}    //namespace s2ne
