#include "original_scripted_movement.h"

namespace s2ne
{
auto Track::advance(const Battle& battle) -> void
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

auto Track::begin(const Battle& battle, const OriginalEvents::Unit& unit, Battle::Point destination, unsigned movement_class, std::shared_ptr<std::array<unsigned, 4>> group_channels) -> bool
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

auto Track::tick(const Battle& battle, unsigned milliseconds) -> void
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

auto Track::active() const -> bool { return active_; }

auto Track::pose() const -> const OriginalEvents::Unit& { return pose_; }

auto Track::path() const -> const std::vector<Battle::Point>& { return path_; }

auto Track::apply(OriginalEvents::Unit& unit) const -> void
{
    unit.x = pose_.x;
    unit.y = pose_.y;
    unit.direction = pose_.direction;
    unit.animation_action = pose_.animation_action;
    unit.movement_frame = pose_.movement_frame;
    unit.movement_pose = pose_.movement_pose;
    unit.action_state = pose_.action_state;
}

auto Group::begin(const Battle& battle, const OriginalEvents& events, const std::vector<std::pair<size_t, Battle::Point>>& roles, unsigned mode) -> bool
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

auto Group::active() const -> bool
{
    return std::any_of(tracks_.begin(), tracks_.end(), [](const auto& t)
        {
            return t.active();
        });
}

auto Group::tick(const Battle& battle, unsigned ms) -> void
{
    for (auto& track : tracks_)
    {
        track.tick(battle, ms);
    }
}

auto Group::apply(std::vector<OriginalEvents::Unit>& units) const -> void
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

auto Group::draw_completed() const -> bool { return mode_ <= 1; }

auto Group::tracks() const -> const std::vector<Track>& { return tracks_; }
}    //namespace s2ne
