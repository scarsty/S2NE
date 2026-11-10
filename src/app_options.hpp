#pragma once
#include <filesystem>
#include <string>
namespace s2ne
{
struct AppOptions
{
    bool verify;
    bool teleportSmoke;
    bool evacuationSmoke;
    bool shopSmoke;
    bool openingRightSmoke;
    bool smoke;
    bool newGameSmoke;
    bool battleSmoke;
    bool alternateSmoke;
    bool autoSmoke;
    bool detailSmoke;
    bool feedbackSmoke;
    bool nataliaContinue;
    bool nataliaStrengthened;
    bool strengthenedFlow;
    bool campaignContinue;
    bool campaignSmoke;
    bool clearSmoke;
    bool loadSmoke;
    bool skill88Input;
    bool skill94Input;
    bool skillInputSmoke;
    bool skill88Positive;
    unsigned inputSkill;
    std::string inputFolder;
    bool skillScheduleStress;
    bool promotionSmoke;
    bool option;
    AppOptions(int argc, char** argv);
    std::filesystem::path resource_root(int argc, char** argv) const;
};
}    //namespace s2ne
