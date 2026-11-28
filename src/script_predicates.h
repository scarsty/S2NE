#pragma once
#include "events.h"
#include <optional>
namespace s2ne
{
// 原45CDF3先主军后对军，按身份首项返回，不按存活或画面可见性替代。
Events::Unit* original_script_role(Events& events, int id);
bool original_death_state_handler_supported(unsigned type, unsigned condition);
Result<bool> original_death_state_handler(Events& events, size_t event);
// 原420A3B的1..4读取WORD+5/+7/+9；2/4仅在谓词成功后清除全部有效操作数。
std::optional<bool> original_flag_predicate(Events& events, size_t event);
//4214C1/4214D5: signed word+5 added with native32-bit wrap.
bool original_currency_handler(Events& events, size_t event);
// 4211A3 conditions0..6. False predicates remain pending, not consumed.
std::optional<bool> original_role_predicate(const Events& events, size_t event);
}    //namespace s2ne
