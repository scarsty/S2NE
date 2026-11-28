#pragma once
#include "original_combat.h"
#include "original_save_bridge.h"
#include "original_settings.h"
namespace s2ne
{
struct OriginalSpellSummonCandidate
{
    OriginalSave::State saved;
    OriginalEvents events;
    unsigned index{};
    uint32_t seed{}, calls{};
    static void finish_action(OriginalEvents::Unit& unit)
    {
        unit.action_state = OriginalSettings::gameplay_assist && !unit.hostile() ? 0 : 2;
        if (unit.action_status_runtime) { unit.action_status_runtime->at(0x2D) = uint8_t(unit.action_state); }
    }
    void resume(std::optional<OriginalSave::State>& destinationSave, std::optional<OriginalEvents>& destinationEvents,
        unsigned& destinationIndex, s2ne::Random& random) const
    {
        destinationSave = saved;
        destinationEvents = events;
        destinationIndex = index;
        random.restore_stream(seed, calls);
    }
};
}    //namespace s2ne
