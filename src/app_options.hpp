#pragma once
#include <filesystem>
#include <string>
namespace s2ne
{
struct AppOptions
{
    bool stdio{};
    bool headlessSpeed{};
    std::filesystem::path stdioSaveRoot, stdioInitialSave;
    AppOptions(int argc, char** argv);
    std::filesystem::path resource_root(int argc, char** argv) const;
};
}    //namespace s2ne
