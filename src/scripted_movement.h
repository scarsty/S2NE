#pragma once
#include "battle.h"
namespace s2ne
{
// 45E107 -> 45E450 ordinary movement. Does not invoke paid battle actions.
class Track
{
    std::vector<Battle::Point> path_;
    Events::Unit pose_;
    size_t cursor_{};
    unsigned elapsed_{};
    bool active_{};
    unsigned sound_channel_{};
    std::shared_ptr<std::array<unsigned, 4>> group_channels_;
    void advance(const Battle& battle);

public:
    bool begin(const Battle& battle, const Events::Unit& unit, Battle::Point destination, unsigned movement_class, std::shared_ptr<std::array<unsigned, 4>> group_channels = {});
    void tick(const Battle& battle, unsigned milliseconds);
    bool active() const;
    const Events::Unit& pose() const;
    const std::vector<Battle::Point>& path() const;
    // Only original movement presentation/coordinates are committed. HP/AP,
    // persistent death/presence, inventory, and every other resource stay intact.
    void apply(Events::Unit& unit) const;
};
// 461EF8 uses the same 8000h graph/chunk extraction as 45E107.
// 461808 advances every live track before its shared 80ms presentation wait.
class Group
{
    std::vector<Track> tracks_;
    std::vector<size_t> actors_;
    unsigned mode_{};
    std::shared_ptr<std::array<unsigned, 4>> sound_channels_;

public:
    bool begin(const Battle& battle, const Events& events, const std::vector<std::pair<size_t, Battle::Point>>& roles, unsigned mode);
    bool active() const;
    void tick(const Battle& battle, unsigned ms);
    void apply(std::vector<Events::Unit>& units) const;
    bool draw_completed() const;
    const std::vector<Track>& tracks() const;
};
}    //namespace s2ne
