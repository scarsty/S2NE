#include "app_options.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>
namespace s2ne
{
AppOptions::AppOptions(int argc, char** argv)
{
    stdio = argc > 1 && std::string_view(argv[1]) == "--stdio";
    if (!stdio)
    {
        if (argc > 2 || (argc > 1 && std::string_view(argv[1]).starts_with("--")))
        {
            throw std::runtime_error("Usage: S2NE [resources] or --stdio <resources> --save-root <new-directory> [--load <save>] [--headless-speed]");
        }
        return;
    }
    if (argc < 5 || std::string_view(argv[3]) != "--save-root")
    {
        throw std::runtime_error("--stdio requires resources and --save-root <new-directory>");
    }
    stdioSaveRoot = std::filesystem::absolute(argv[4]);
    for (int i = 5; i < argc; ++i)
    {
        const std::string_view argument(argv[i]);
        if (argument == "--headless-speed" && !headlessSpeed) { headlessSpeed = true; }
        else if (argument == "--load" && stdioInitialSave.empty() && i + 1 < argc)
        {
            stdioInitialSave = std::filesystem::absolute(argv[++i]);
        }
        else
        {
            throw std::runtime_error("Unexpected or repeated --stdio argument");
        }
    }
}
std::filesystem::path AppOptions::resource_root(int argc, char** argv) const
{
    const int resourceArgument = stdio ? 2 : 1;
    if (argc > resourceArgument) { return std::filesystem::u8path(argv[resourceArgument]); }
    auto root = std::filesystem::current_path();
    if (std::filesystem::exists(root / "SUPERSP.PAL")) { return root; }
    auto candidate = std::filesystem::u8path(SDL_GetBasePath());
    while (!candidate.empty())
    {
        if (std::filesystem::exists(candidate / "SUPERSP.PAL")) { return candidate; }
        const auto parent = candidate.parent_path();
        if (parent == candidate) { break; }
        candidate = parent;
    }
    return root;
}
}    //namespace s2ne
