#pragma once
#include "resource_bundle.h"
#include "resource_result.h"
#include <array>
#include <cstdio>
namespace s2ne
{
Result<std::filesystem::path> original_music_path(const std::filesystem::path& root, unsigned id, ResourceDomain domain);
}    //namespace s2ne
