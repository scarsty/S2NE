#include "app_runtime.hpp"
#include "runtime_diagnostics.hpp"
#include <SDL3/SDL_main.h>
int main(int argc, char** argv)
{
    try
    {
        return s2ne::run_application(argc, argv);
    } catch (...) { return s2ne::report_unexpected_exception(); }
}
