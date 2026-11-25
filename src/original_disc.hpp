#pragma once
#include "resource_bundle.hpp"
namespace s2ne
{
constexpr ResourceDomain resource_domain_for_campaign(int route, int progress, int stage_global)
{
    // Original campaign globals select a script set, not a storage device.
    return (route == 1 && progress >= 7) || (route == 0 && (stage_global == 60 || stage_global == 61)) ? ResourceDomain::secondary : ResourceDomain::primary;
}
}    //namespace s2ne
