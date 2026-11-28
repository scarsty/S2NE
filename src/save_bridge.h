#pragma once
#include "events.h"
#include "save.h"
#include <optional>
#include <utility>

namespace s2ne
{
class SaveBridge
{
    friend class Initialization;
    friend class Church;
    friend Result<ResourceDomain> try_app_domain_for_save(const Save::State& state);
    friend ResourceDomain update_app_domain_for_stage(Save::State& state);
    using Raw = Save::UnitRecord;
    static uint32_t get(std::span<const uint8_t> b, size_t p, unsigned n = 2);
    static int signed_word(const Raw& r, size_t p);
    static void put(std::span<uint8_t> b, size_t p, uint32_t v, unsigned n = 2);

    static Result<bool> try_validate(const Save::State& raw);
    static void validate(const Save::State& raw);
    static void decode_unit(const Raw& r, Events::Unit& u);
    static Result<bool> try_patch_unit(Raw& r, const Events::Unit& u);
    static void patch_unit(Raw& record, const Events::Unit& unit);

public:
    static void grant_scripted_item(Save::State& candidate, unsigned item);
    static void order_third_route_roster(Save::State& state);
    static void compact_interbattle_members(Save::State& state);
    struct Restored
    {
        Events events;
        unsigned turn;
        std::pair<int, int> camera;
        Save::State raw;
    };
    // Camera is not in sub_4362B4's serialized fields. Original map reload resets it.
    static constexpr std::pair<int, int> restored_camera{ 32, 16 };
    static Result<Restored> try_restore(const std::filesystem::path& root, const Save::State& input);
    static Restored restore(const std::filesystem::path& root, const Save::State& raw);
    static Result<Save::State> try_capture_promotion(const Events& events, unsigned turn, std::pair<int, int> camera,
        const Save::State& baseline, size_t selected, unsigned source_kind,
        std::span<const uint8_t> inventory, uint8_t army_metadata);
    static Result<Save::State> try_capture(const Events& events, unsigned turn, std::pair<int, int> camera,
        const Save::State* baseline = nullptr);
    static Save::State capture(const Events& events, unsigned turn, std::pair<int, int> camera, const Save::State* baseline = nullptr);
    static Save::State capture_promotion(const Events& events, unsigned turn, std::pair<int, int> camera, const Save::State& baseline, size_t selected, unsigned source_kind, std::span<const uint8_t> inventory, uint8_t metadata);
};
}    //namespace s2ne
