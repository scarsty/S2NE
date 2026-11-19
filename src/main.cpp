#include "app_runtime.hpp"
#include "runtime_diagnostics.hpp"
#include "terminal_output.hpp"
#include "texture_experiment.hpp"
#include <SDL3/SDL_main.h>
#include <exception>
int main(int argc, char** argv)
{
    try
    {
        if (argc > 1 && std::string_view(argv[1]) == "--texture-experiment")
        {
            if (argc < 4 || argc > 5 || (argc == 5 && std::string_view(argv[4]) != "--smoke")) { throw std::runtime_error("Usage: --texture-experiment <export-directory> <visual-id> [--smoke]"); }
            return s2ne::run_texture_experiment(argv[2], unsigned(std::stoul(argv[3])), argc == 5);
        }
        return s2ne::run_application(argc, argv);
    } catch (const std::exception& error)
    {
        std::print(stderr, "S2NE: uncaught std::exception: {}\n", error.what());
        s2ne::print_stack(s2ne::capture_stack());
        return s2ne::report_unexpected_exception();
    } catch (...)
    {
        s2ne::print_stack(s2ne::capture_stack());
        return s2ne::report_unexpected_exception();
    }
}
