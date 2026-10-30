#ifndef LODESTAR_COMMAND_HANDLER_HPP
#define LODESTAR_COMMAND_HANDLER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/global_functor_name_printer.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/pud_focus.hpp"
#include "infrastructure/pud_loaded_modules.hpp"
#include "infrastructure/pud_lodestar_runtime.hpp"
#include "infrastructure/pud_solve_loop.hpp"
#include "infrastructure/pud_tick_progress.hpp"
#include "infrastructure/solve_timer.hpp"
#include "infrastructure/steady_now.hpp"
#include "value_objects/pud_global_node_id.hpp"

struct lodestar_command_handler {
    lodestar_command_handler(
        const std::vector<std::string>& module_paths,
        const std::string& focus_text,
        size_t max_unfolds,
        size_t non_conjecture_unfolds_per_conjecture_unfold,
        size_t max_node_depth,
        size_t sim_progress_interval);
    void operator()();
private:
    void require_focus_node();

    using functor_printer_t = global_functor_name_printer<pud_lodestar_runtime>;
    using printer_t = expr_printer<id_var_printer, functor_printer_t>;
    using timer_t = solve_timer<steady_now>;
    using progress_t = pud_tick_progress<timer_t>;
    using loop_t = pud_solve_loop<
        pud_lodestar_runtime, printer_t, progress_t, progress_t, progress_t, timer_t, timer_t>;

    pud_loaded_modules loaded_;
    pud_global_node_id focus_;
    pud_lodestar_runtime runtime_;
    id_var_printer print_var_;
    functor_printer_t print_functor_;
    printer_t printer_;
    steady_now clock_;
    timer_t solve_timer_;
    progress_t progress_;
    loop_t solve_loop_;
};

#endif
