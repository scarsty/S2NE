#pragma once

namespace s2ne
{
inline constexpr bool is_custom_stage(unsigned id) { return id >= 201; }
inline constexpr unsigned original_stage_id(unsigned id) { return is_custom_stage(id) ? id - 200 : id; }
}    // namespace s2ne
