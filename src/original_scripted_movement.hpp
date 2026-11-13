#pragma once
#include "battle.hpp"
namespace s2ne::original_scripted_movement
{
// 45E107 -> 45E450 ordinary movement. Does not invoke paid battle actions.
class Track
{
    std::vector<Battle::Point> path_;
    OriginalEvents::Unit pose_;
    size_t cursor_{};
    unsigned elapsed_{};
    bool active_{};
    unsigned sound_channel_{};
    std::shared_ptr<std::array<unsigned, 4>> group_channels_;
    void advance(const Battle& battle)
    {
        auto next = path_.at(cursor_);
        unsigned direction = Battle::facing({ pose_.x, pose_.y }, next, pose_.direction);
        unsigned frames = battle.scripted_frame_count(pose_, direction);
        pose_.movement_frame = direction == pose_.direction && frames ? (pose_.movement_frame + 1) % frames : 0;
        pose_.direction = direction;
        pose_.x = next.x;
        pose_.y = next.y;
        const auto id = battle.scripted_movement_sound_id(pose_);
        // 原45E450共用一个返回通道；461808则在全组角色间按四个分支各自复用。
        unsigned* channel = &sound_channel_;
        if (group_channels_)
        {
            unsigned branch = 3;
            if (id == 122) { branch = 0; }
            else if (id == 131) { branch = 1; }
            else if (id == 124) { branch = 2; }
            channel = &group_channels_->at(branch);
        }
        *channel = battle.scripted_effect_sound(id, *channel);
    }

public:
    bool begin(const Battle& battle, const OriginalEvents::Unit& unit, Battle::Point destination, unsigned movement_class, std::shared_ptr<std::array<unsigned, 4>> group_channels = {})
    {
        if (active_ || movement_class > 1)
        {
            return false;
        }
        auto path = battle.scripted_route(unit, destination);
        if (!path)
        {
            return false;
        }
        path_ = std::move(*path);
        pose_ = unit;
        cursor_ = elapsed_ = 0;
        sound_channel_ = 0;
        group_channels_ = group_channels;
        active_ = !path_.empty();
        pose_.animation_action = 1;
        pose_.movement_frame = 0;
        pose_.movement_pose = active_;
        pose_.action_state = active_ ? 3 : 0;
        if (active_)
        {
            advance(battle);
        }
        return true;
    }
    void tick(const Battle& battle, unsigned milliseconds)
    {
        if (!active_)
        {
            return;
        }
        elapsed_ += std::min(milliseconds, 80u);
        if (elapsed_ < 80)
        {
            return;
        }
        elapsed_ = 0;
        if (++cursor_ == path_.size())
        {
            active_ = false;
            pose_.animation_action = 0;
            pose_.movement_frame = 0;
            pose_.movement_pose = false;
            pose_.action_state = 0;
            return;
        }
        advance(battle);
    }
    bool active() const { return active_; }
    const OriginalEvents::Unit& pose() const { return pose_; }
    const std::vector<Battle::Point>& path() const { return path_; }
    // Only original movement presentation/coordinates are committed. HP/AP,
    // persistent death/presence, inventory, and every other resource stay intact.
    void apply(OriginalEvents::Unit& unit) const
    {
        unit.x = pose_.x;
        unit.y = pose_.y;
        unit.direction = pose_.direction;
        unit.animation_action = pose_.animation_action;
        unit.movement_frame = pose_.movement_frame;
        unit.movement_pose = pose_.movement_pose;
        unit.action_state = pose_.action_state;
    }
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
    bool begin(const Battle& battle, const OriginalEvents& events, const std::vector<std::pair<size_t, Battle::Point>>& roles, unsigned mode)
    {
        if (active() || mode > 3)
        {
            return false;
        }
        std::vector<Track> tracks;
        std::vector<size_t> actors;
        sound_channels_ = std::make_shared<std::array<unsigned, 4>>();
        for (auto [index, destination] : roles)
        {
            if (index >= events.units.size())
            {
                return false;
            }
            Track track;
            // 4619AB: a failed461EF8 route removes only this actor from the parallel list.
            if (events.units[index].status_flags & 0x20)
            {
                continue;
            }
            if (!track.begin(battle, events.units[index], destination, events.movement_class(events.units[index]), sound_channels_))
            {
                continue;
            }
            tracks.push_back(std::move(track));
            actors.push_back(index);
        }
        tracks_ = std::move(tracks);
        actors_ = std::move(actors);
        mode_ = mode;
        return true;
    }
    bool active() const
    {
        return std::any_of(tracks_.begin(), tracks_.end(), [](const auto& t)
            {
                return t.active();
            });
    }
    void tick(const Battle& battle, unsigned ms)
    {
        for (auto& track : tracks_)
        {
            track.tick(battle, ms);
        }
    }
    void apply(std::vector<OriginalEvents::Unit>& units) const
    {
        for (size_t i = 0; i < tracks_.size(); ++i)
        {
            auto& u = units.at(actors_[i]);
            tracks_[i].apply(u);
            if (mode_ > 1 && !tracks_[i].active())
            {
                u.action_state = 3;
            }
        }
    }
    bool draw_completed() const { return mode_ <= 1; }
    const std::vector<Track>& tracks() const { return tracks_; }
};
}    //namespace s2ne::original_scripted_movement
