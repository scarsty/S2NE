#include "app_runtime.h"
#include "runtime_diagnostics.h"
#include "terminal_output.h"
#include <SDL3/SDL_main.h>
#include <exception>
int main(int argc, char** argv)
{
    try
    {
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
