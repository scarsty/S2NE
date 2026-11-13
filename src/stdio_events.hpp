#pragma once
#include <SDL3/SDL_events.h>
#include <deque>
#include <vector>
namespace s2ne
{
inline std::deque<SDL_Event> stdio_event_order(const std::vector<SDL_Event>& commands, const std::vector<SDL_Event>& ordinary)
{
    std::deque<SDL_Event> result;
    for (const auto& event : ordinary)
    {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) { result.push_back(event); }
    }
    result.insert(result.end(), commands.begin(), commands.end());
    for (const auto& event : ordinary)
    {
        if (event.type != SDL_EVENT_QUIT && event.type != SDL_EVENT_WINDOW_CLOSE_REQUESTED) { result.push_back(event); }
    }
    return result;
}
}    //namespace s2ne
