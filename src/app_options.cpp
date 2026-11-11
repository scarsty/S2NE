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
    const bool curse33Input = argc > 1 && std::string(argv[1]) == "--smoke-skill33-input";
    const bool timedBuffInput = argc > 1 && (std::string(argv[1]) == "--smoke-skill37-input" || std::string(argv[1]) == "--smoke-skill44-input" || std::string(argv[1]) == "--smoke-skill46-input" || std::string(argv[1]) == "--smoke-skill58-input" || std::string(argv[1]) == "--smoke-skill86-input" || std::string(argv[1]) == "--smoke-skill92-input");
    const bool abilityBuffInput = argc > 1 && (std::string(argv[1]) == "--smoke-skill22-input" || std::string(argv[1]) == "--smoke-skill23-input" || std::string(argv[1]) == "--smoke-skill28-input" || std::string(argv[1]) == "--smoke-skill42-input" || std::string(argv[1]) == "--smoke-skill47-input" || std::string(argv[1]) == "--smoke-skill57-input" || std::string(argv[1]) == "--smoke-skill76-input" || std::string(argv[1]) == "--smoke-skill78-input" || std::string(argv[1]) == "--smoke-skill79-input" || std::string(argv[1]) == "--smoke-skill84-input" || std::string(argv[1]) == "--smoke-skill89-input" || std::string(argv[1]) == "--smoke-skill91-input");
    const bool specialStatusInput = abilityBuffInput || timedBuffInput || argc > 1 && (std::string(argv[1]) == "--smoke-skill8-input" || std::string(argv[1]) == "--smoke-skill12-input" || std::string(argv[1]) == "--smoke-skill35-input" || std::string(argv[1]) == "--smoke-skill39-input" || std::string(argv[1]) == "--smoke-skill48-input" || std::string(argv[1]) == "--smoke-skill56-input" || std::string(argv[1]) == "--smoke-skill60-input");
    const bool hostileStatusInput = specialStatusInput || argc > 1 && (std::string(argv[1]) == "--smoke-skill17-input" || std::string(argv[1]) == "--smoke-skill25-input" || std::string(argv[1]) == "--smoke-skill52-input" || std::string(argv[1]) == "--smoke-skill61-input" || std::string(argv[1]) == "--smoke-skill13-input" || std::string(argv[1]) == "--smoke-skill15-input" || std::string(argv[1]) == "--smoke-skill53-input" || std::string(argv[1]) == "--smoke-skill34-input" || std::string(argv[1]) == "--smoke-skill38-input" || std::string(argv[1]) == "--smoke-skill54-input" || std::string(argv[1]) == "--smoke-skill96-input");
    const bool restorationInput = argc > 1 && (std::string(argv[1]) == "--smoke-skill6-input" || std::string(argv[1]) == "--smoke-skill9-input" || std::string(argv[1]) == "--smoke-skill10-input");
    const bool status4Input = argc > 1 && std::string(argv[1]) == "--smoke-skill4-input";
    const bool status5Input = argc > 1 && std::string(argv[1]) == "--smoke-skill5-input";
    skillControl = argc > 5 && (std::string(argv[1]) == "--smoke-skill2-input" || std::string(argv[1]) == "--smoke-skill3-input" || status4Input || status5Input || restorationInput || hostileStatusInput || skill94Input) ? argv[5] : "";
    const bool blessing3Input = argc > 1 && std::string(argv[1]) == "--smoke-skill3-input";
    const bool sleep2Input = argc > 1 && std::string(argv[1]) == "--smoke-skill2-input";
    const bool healing1Input = argc > 1 && std::string(argv[1]) == "--smoke-skill1-input";
    skillInputSmoke = hostileStatusInput || restorationInput || status4Input || status5Input || blessing3Input || sleep2Input || healing1Input || curse33Input || skill94Input || skill88Input || (argc > 1 && std::string(argv[1]) == "--smoke-skill32-input");
    skill88Positive = skill88Input && argc > 4 && std::string(argv[4]) == "--positive-heal";
    unsigned int requestedSkill;
    if (specialStatusInput) { requestedSkill = unsigned(std::stoul(std::string(argv[1]).substr(13))); }
    else if (hostileStatusInput)
    {
        if (std::string(argv[1]) == "--smoke-skill17-input") { requestedSkill = 17; }
        else if (std::string(argv[1]) == "--smoke-skill25-input") { requestedSkill = 25; }
        else if (std::string(argv[1]) == "--smoke-skill52-input") { requestedSkill = 52; }
        else if (std::string(argv[1]) == "--smoke-skill61-input") { requestedSkill = 61; }
        else if (std::string(argv[1]) == "--smoke-skill96-input") { requestedSkill = 96; }
        else if (std::string(argv[1]) == "--smoke-skill34-input") { requestedSkill = 34; }
        else if (std::string(argv[1]) == "--smoke-skill38-input") { requestedSkill = 38; }
        else if (std::string(argv[1]) == "--smoke-skill54-input") { requestedSkill = 54; }
        else if (std::string(argv[1]) == "--smoke-skill13-input") { requestedSkill = 13; }
        else if (std::string(argv[1]) == "--smoke-skill15-input") { requestedSkill = 15; }
        else
        {
            requestedSkill = 53;
        }
    }
    else if (restorationInput)
    {
        if (std::string(argv[1]) == "--smoke-skill6-input") { requestedSkill = 6; }
        else if (std::string(argv[1]) == "--smoke-skill9-input") { requestedSkill = 9; }
        else
        {
            requestedSkill = 10;
        }
    }
    else if (status4Input) { requestedSkill = 4; }
    else if (status5Input) { requestedSkill = 5; }
    else if (blessing3Input) { requestedSkill = 3; }
    else if (sleep2Input) { requestedSkill = 2; }
    else if (healing1Input) { requestedSkill = 1; }
    else if (curse33Input) { requestedSkill = 33; }
    else if (skill94Input) { requestedSkill = 94; }
    else if (skill88Input) { requestedSkill = 88; }
    else
    {
        requestedSkill = 32;
    }

    inputSkill = requestedSkill;
    const char* requestedFolder;
    if (hostileStatusInput)
    {
        if (argc > 4) { requestedFolder = argv[4]; }
        else
        {
            requestedFolder = "hostile-status-input";
        }
    }
    else if (restorationInput)
    {
        if (argc > 4) { requestedFolder = argv[4]; }
        else
        {
            requestedFolder = "restoration-input";
        }
    }
    else if ((status4Input || status5Input))
    {
        if (argc > 4) { requestedFolder = argv[4]; }
        else if (status4Input) { requestedFolder = "skill4-input"; }
        else
        {
            requestedFolder = "skill5-input";
        }
    }
    else if (blessing3Input)
    {
        if (argc > 4) { requestedFolder = argv[4]; }
        else
        {
            requestedFolder = "skill3-input";
        }
    }
    else if (sleep2Input)
    {
        if (argc > 4) { requestedFolder = argv[4]; }
        else
        {
            requestedFolder = "skill2-input";
        }
    }
    else if (healing1Input) { requestedFolder = "skill1-input"; }
    else if (curse33Input) { requestedFolder = "skill33-input"; }
    else if (skill94Input)
    {
        if (argc > 4) { requestedFolder = argv[4]; }
        else
        {
            requestedFolder = "skill94-input-next";
        }
    }
    else if (skill88Positive) { requestedFolder = "skill88-positive-input"; }
    else if (skill88Input) { requestedFolder = "skill88-input"; }
    else
    {
        requestedFolder = "skill32-input";
    }

    inputFolder = requestedFolder;
    if (skillControl != "mixed" && skillControl != "rollback" && !(restorationInput && skillControl == "branches") && !(skill94Input && (skillControl == "kill" || skillControl == "revive25" || skillControl == "consume28" || skillControl == "reverse29" || skillControl == "script82" || skillControl == "scriptfail82"))) { skillControl.clear(); }
    skillScheduleStress = skillInputSmoke && argc > 4 && std::string(argv[4]) == "--schedule-stress";
    promotionSmoke = argc > 1 && std::string(argv[1]) == "--smoke-promotion";
    option = teleportSmoke || evacuationSmoke || shopSmoke || verify || smoke || newGameSmoke || battleSmoke || loadSmoke || promotionSmoke || skillInputSmoke;
}

std::filesystem::path AppOptions::resource_root(int argc, char** argv) const
{
    int resource_argument = 1;
    if (option) { resource_argument = 2; }
    std::filesystem::path root;
    if (argc > resource_argument) { root = std::filesystem::u8path(argv[resource_argument]); }
    else
    {
        root = std::filesystem::current_path();
    }
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
