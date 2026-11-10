#include "app_options.hpp"
#include <SDL3/SDL.h>
namespace s2ne
{
AppOptions::AppOptions(int argc, char** argv)
{
    verify = argc > 1 && std::string(argv[1]) == "--verify";
    teleportSmoke = argc > 1 && std::string(argv[1]) == "--smoke-teleport";
    evacuationSmoke = argc > 1 && std::string(argv[1]) == "--smoke-evacuation";
    shopSmoke = argc > 1 && std::string(argv[1]) == "--smoke-shop";
    openingRightSmoke = argc > 1 && std::string(argv[1]) == "--smoke-opening-right-skip";
    smoke = openingRightSmoke || (argc > 1 && std::string(argv[1]) == "--smoke");
    newGameSmoke = argc > 1 && std::string(argv[1]) == "--smoke-new-game";
    freshSaveSmoke = argc > 1 && std::string(argv[1]) == "--smoke-fresh-save";
    battleSmoke = freshSaveSmoke || (argc > 1 && (std::string(argv[1]) == "--smoke-battle" || std::string(argv[1]) == "--smoke-battle-alternate"));
    alternateSmoke = battleSmoke && std::string(argv[1]) == "--smoke-battle-alternate";
    autoSmoke = argc > 1 && (std::string(argv[1]) == "--smoke-auto" || std::string(argv[1]) == "--smoke-auto-alternate");
    if (autoSmoke && std::string(argv[1]) == "--smoke-auto-alternate")
    {
        alternateSmoke = true;
    }
    detailSmoke = argc > 1 && std::string(argv[1]) == "--smoke-detail";
    feedbackSmoke = argc > 1 && std::string(argv[1]) == "--smoke-feedback";
    nataliaContinue = argc > 1 && std::string(argv[1]) == "--smoke-campaign-strengthened-alternate-continue";
    nataliaStrengthened = nataliaContinue || (argc > 1 && std::string(argv[1]) == "--smoke-campaign-strengthened-alternate");
    strengthenedFlow = nataliaStrengthened || (argc > 1 && std::string(argv[1]) == "--smoke-campaign-strengthened");
    if (nataliaStrengthened)
    {
        alternateSmoke = true;
    }
    campaignContinue = strengthenedFlow || (argc > 1 && std::string(argv[1]) == "--smoke-campaign-continue");
    campaignSmoke = campaignContinue || (argc > 1 && (std::string(argv[1]) == "--smoke-campaign" || std::string(argv[1]) == "--smoke-campaign-alternate"));
    if (campaignSmoke && std::string(argv[1]) == "--smoke-campaign-alternate")
    {
        alternateSmoke = true;
    }
    clearSmoke = campaignSmoke || (argc > 1 && std::string(argv[1]) == "--smoke-clear");
    autoSmoke = autoSmoke || feedbackSmoke;
    battleSmoke = battleSmoke || autoSmoke || detailSmoke || clearSmoke;
    loadSmoke = argc > 1 && std::string(argv[1]) == "--smoke-load";
    skill88Input = argc > 1 && std::string(argv[1]) == "--smoke-skill88-input";
    skill94Input = argc > 1 && std::string(argv[1]) == "--smoke-skill94-input";
    skillInputSmoke = skill94Input || skill88Input || (argc > 1 && std::string(argv[1]) == "--smoke-skill32-input");
    skill88Positive = skill88Input && argc > 4 && std::string(argv[4]) == "--positive-heal";
    inputSkill = skill94Input ? 94 : skill88Input ? 88 :
                                                    32;
    inputFolder = skill94Input ? (argc > 4 ? argv[4] : "skill94-input-next") : skill88Positive ? "skill88-positive-input" :
        skill88Input                                                                           ? "skill88-input" :
                                                                                                 "skill32-input";
    skillScheduleStress = skillInputSmoke && argc > 4 && std::string(argv[4]) == "--schedule-stress";
    promotionSmoke = argc > 1 && std::string(argv[1]) == "--smoke-promotion";
    option = teleportSmoke || evacuationSmoke || shopSmoke || verify || smoke || newGameSmoke || battleSmoke || loadSmoke || promotionSmoke || skillInputSmoke;
}

std::filesystem::path AppOptions::resource_root(int argc, char** argv) const
{
    auto root = argc > (option ? 2 : 1) ? std::filesystem::u8path(argv[option ? 2 : 1]) : std::filesystem::current_path();
    if (argc <= (option ? 2 : 1) && !std::filesystem::exists(root / "SUPERSP.PAL"))
    {
        auto candidate = std::filesystem::u8path(SDL_GetBasePath());
        while (!candidate.empty())
        {
            if (std::filesystem::exists(candidate / "SUPERSP.PAL"))
            {
                root = candidate;
                break;
            }
            auto parent = candidate.parent_path();
            if (parent == candidate)
            {
                break;
            }
            candidate = parent;
        }
    }
    return root;
}
}    //namespace s2ne
