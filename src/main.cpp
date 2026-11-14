#include "app_runtime.hpp"
#include "runtime_diagnostics.hpp"
#include <SDL3/SDL_main.h>
#include <exception>
int main(int argc, char** argv)
{
    try
    {
        return s2ne::run_application(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "S2NE: uncaught std::exception: %s\n", error.what());
        return s2ne::report_unexpected_exception();
    }
    catch (...)
    {
        return s2ne::report_unexpected_exception();
    }
}
