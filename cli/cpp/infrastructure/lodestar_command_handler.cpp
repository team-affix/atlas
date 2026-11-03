#include "infrastructure/lodestar_command_handler.hpp"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

lodestar_command_handler::lodestar_command_handler(
    const std::vector<std::string>& module_paths,
    const std::string& focus_text,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold,
    size_t max_node_depth,
    size_t max_open_goals,
    size_t sim_progress_interval,
    size_t unfolds_per_save,
    bool report_stored_solutions)
    : loaded_(module_paths)
    , focus_(pud_focus_parser{}.parse(focus_text))
    , runtime_(
          loaded_.exprs(),
          focus_,
          max_unfolds,
          non_conjecture_unfolds_per_conjecture_unfold,
          max_node_depth,
          max_open_goals,
          report_stored_solutions)
    , print_var_()
    , print_functor_(runtime_)
    , printer_(std::cout, print_var_, print_functor_)
    , clock_()
    , solve_timer_(clock_)
    , progress_(solve_timer_, runtime_, runtime_, runtime_, isatty(fileno(stdout)) != 0)
    , autosaver_(runtime_, loaded_, runtime_, unfolds_per_save)
    , solve_loop_(
          runtime_, runtime_, runtime_, runtime_, runtime_, printer_, progress_, progress_, progress_, solve_timer_,
          solve_timer_, autosaver_, autosaver_, sim_progress_interval) {
    bool focus_loaded = false;
    for (const pud_module& module : loaded_.modules()) {
        if (module.get_module_id() != focus_.module)
            continue;
        if (focus_.node >= module.count_nodes())
            throw std::runtime_error("focus node is outside the module");
        focus_loaded = true;
    }
    if (!focus_loaded)
        throw std::runtime_error("focus module was not loaded");
    for (pud_module& module : loaded_.modules())
        runtime_.register_module(module);
}

void lodestar_command_handler::operator()() {
    solve_loop_.run();
}
