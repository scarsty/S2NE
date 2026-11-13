#pragma once
#include "original_combat.hpp"
#include "original_save_bridge.hpp"
namespace s2ne
{
struct OriginalSpellSummonCandidate
{
    OriginalSave::State saved;
    OriginalEvents events;
    unsigned index{};
    uint32_t seed{}, calls{};
    void resume(std::optional<OriginalSave::State>& destinationSave, std::optional<OriginalEvents>& destinationEvents,
        unsigned& destinationIndex, original_combat::Random& random) const
    {
        destinationSave = saved;
        destinationEvents = events;
        destinationIndex = index;
        random.restore_stream(seed, calls);
    }
};
}    //namespace s2ne
