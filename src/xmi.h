#pragma once
#include "resource_result.h"
#include <algorithm>
namespace s2ne
{
// XMI延时是连续低位字节之和，音符时值才是VLQ；固定120Hz不随原tempo事件变速。
Result<std::vector<uint8_t>> original_xmi_midi(std::span<const uint8_t> xmi);
Result<std::vector<std::vector<uint8_t>>> original_xmi_resources(const std::filesystem::path& root);
}    //namespace s2ne
