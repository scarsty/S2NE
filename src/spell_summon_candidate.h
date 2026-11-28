#pragma once
#include "combat.h"
#include "save_bridge.h"
#include "settings.h"
namespace s2ne
{
struct SpellSummonCandidate
{
    Save::State saved;
    Events events;
    unsigned index{};
    uint32_t seed{}, calls{};
    static void finish_action(Events::Unit& unit)
    {
        unit.action_state = Settings::gameplay_assist && !unit.hostile() ? 0 : 2;
        if (unit.action_status_runtime) { unit.action_status_runtime->at(0x2D) = uint8_t(unit.action_state); }
    }
    void resume(std::optional<Save::State>& destinationSave, std::optional<Events>& destinationEvents,
        unsigned& destinationIndex, s2ne::Random& random) const
    {
        destinationSave = saved;
        destinationEvents = events;
        destinationIndex = index;
        random.restore_stream(seed, calls);
    }
};
}    //namespace s2ne
