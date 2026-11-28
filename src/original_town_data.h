#pragma once
#include "resource_result.h"
#include "text_encoding.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace s2ne
{
Result<std::vector<uint8_t>> try_load_towns_json(const std::filesystem::path& root);
}    //namespace s2ne
