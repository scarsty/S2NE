#pragma once
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <vector>
#ifdef __ANDROID__
#include <SDL3/SDL_system.h>
#include <jni.h>
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace s2ne
{
std::optional<std::vector<uint8_t>> convert_text(std::span<const uint8_t> input, bool to_big5);
bool utf8_to_big5(const std::string& input, std::vector<uint8_t>& output);
}    // namespace s2ne
