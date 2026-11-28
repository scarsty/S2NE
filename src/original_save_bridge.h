#pragma once
#include "original_events.h"
#include "original_save.h"
#include <optional>
#include <utility>

namespace s2ne
{
class OriginalSaveBridge
{
    friend class OriginalInitialization;
    friend class OriginalChurch;
    friend Result<ResourceDomain> try_app_domain_for_save(const OriginalSave::State& state);
    friend ResourceDomain update_app_domain_for_stage(OriginalSave::State& state);
    using Raw = OriginalSave::UnitRecord;
    static uint32_t get(std::span<const uint8_t> b, size_t p, unsigned n = 2);
    static int signed_word(const Raw& r, size_t p);
    static void put(std::span<uint8_t> b, size_t p, uint32_t v, unsigned n = 2);

    static Result<bool> try_validate(const OriginalSave::State& raw);
    static void validate(const OriginalSave::State& raw);
    static void decode_unit(const Raw& r, OriginalEvents::Unit& u);
    static Result<bool> try_patch_unit(Raw& r, const OriginalEvents::Unit& u);
    static void patch_unit(Raw& record, const OriginalEvents::Unit& unit);

public:
    static void grant_scripted_item(OriginalSave::State& candidate, unsigned item);
    static void order_third_route_roster(OriginalSave::State& state);
    static void compact_interbattle_members(OriginalSave::State& state);
    struct Restored
    {
        OriginalEvents events;
        unsigned turn;
        std::pair<int, int> camera;
        OriginalSave::State raw;
    };
    // Camera is not in sub_4362B4's serialized fields. Original map reload resets it.
    static constexpr std::pair<int, int> restored_camera{ 32, 16 };
    static Result<Restored> try_restore(const std::filesystem::path& root, const OriginalSave::State& input);
    static Restored restore(const std::filesystem::path& root, const OriginalSave::State& raw);
    static Result<OriginalSave::State> try_capture_promotion(const OriginalEvents& events, unsigned turn, std::pair<int, int> camera,
        const OriginalSave::State& baseline, size_t selected, unsigned source_kind,
        std::span<const uint8_t> inventory, uint8_t army_metadata);
    static Result<OriginalSave::State> try_capture(const OriginalEvents& events, unsigned turn, std::pair<int, int> camera,
        const OriginalSave::State* baseline = nullptr);
    static OriginalSave::State capture(const OriginalEvents& events, unsigned turn, std::pair<int, int> camera, const OriginalSave::State* baseline = nullptr);
    static OriginalSave::State capture_promotion(const OriginalEvents& events, unsigned turn, std::pair<int, int> camera, const OriginalSave::State& baseline, size_t selected, unsigned source_kind, std::span<const uint8_t> inventory, uint8_t metadata);
};
}    //namespace s2ne
