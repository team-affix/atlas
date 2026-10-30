#include "infrastructure/lodestar_command_handler.hpp"

#include <iostream>
#include <stdexcept>

lodestar_command_handler::lodestar_command_handler(
    const std::vector<std::string>& module_paths,
    const std::string& focus_text,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold,
    size_t max_node_depth,
    size_t sim_progress_interval)
    : loaded_(module_paths)
    , focus_(pud_focus{}.parse(focus_text))
    , runtime_(
          loaded_.exprs(),
          focus_,
          max_unfolds,
          non_conjecture_unfolds_per_conjecture_unfold,
          max_node_depth)
    , print_var_()
    , print_functor_(runtime_)
    , printer_(std::cout, print_var_, print_functor_)
    , clock_()
    , solve_timer_(clock_)
    , progress_(solve_timer_)
    , solve_loop_(progress_, progress_, progress_, solve_timer_, solve_timer_, sim_progress_interval) {
    require_focus_node();
    for (pud_module& module : loaded_.modules())
        runtime_.register_module(module);
}

void lodestar_command_handler::operator()() {
    solve_loop_.run(runtime_, printer_);
}

void lodestar_command_handler::require_focus_node() {
    for (const pud_module& module : loaded_.modules()) {
        if (module.get_module_id() != focus_.module)
            continue;
        if (focus_.node >= module.count_nodes())
            throw std::runtime_error("focus node is outside the module");
        return;
    }
    throw std::runtime_error("focus module was not loaded");
}
