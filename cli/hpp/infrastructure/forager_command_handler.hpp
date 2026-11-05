#ifndef FORAGER_COMMAND_HANDLER_HPP
#define FORAGER_COMMAND_HANDLER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/global_functor_name_printer.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/pud_focus_parser.hpp"
#include "infrastructure/pud_load_readout.hpp"
#include "infrastructure/pud_loaded_modules.hpp"
#include "infrastructure/pud_forager_runtime.hpp"
#include "infrastructure/pud_module_autosaver.hpp"
#include "infrastructure/pud_save_time_recorder.hpp"
#include "infrastructure/pud_solve_loop.hpp"
#include "infrastructure/pud_unfold_progress.hpp"
#include "infrastructure/solve_timer.hpp"
#include "infrastructure/steady_now.hpp"
#include "value_objects/pud_global_node_id.hpp"

struct forager_command_handler {
    forager_command_handler(
        const std::vector<std::string>& module_paths,
        const std::string& focus_text,
        size_t max_unfolds,
        size_t cascade_depth,
        size_t max_node_depth,
        size_t max_open_goals,
        size_t sim_progress_interval,
        size_t unfolds_per_save,
        bool report_stored_solutions,
        bool quiet);
    void operator()();
private:
    using functor_printer_t = global_functor_name_printer<pud_forager_runtime>;
    using printer_t = expr_printer<id_var_printer, functor_printer_t>;
    using readout_t = pud_load_readout<steady_now>;
    using timer_t = solve_timer<steady_now>;
    using save_recorder_t = pud_save_time_recorder<steady_now, pud_loaded_modules>;
    using progress_t = pud_unfold_progress<
        timer_t, pud_forager_runtime, pud_forager_runtime, pud_forager_runtime, save_recorder_t>;
    using autosaver_t = pud_module_autosaver<pud_forager_runtime, save_recorder_t, pud_forager_runtime>;
    using loop_t = pud_solve_loop<
        pud_forager_runtime, pud_forager_runtime, pud_forager_runtime, pud_forager_runtime, pud_forager_runtime,
        printer_t, progress_t, progress_t, progress_t, timer_t, timer_t, autosaver_t, autosaver_t>;

    std::vector<std::string> module_paths_;
    pud_loaded_modules loaded_;
    pud_global_node_id focus_;
    pud_forager_runtime runtime_;
    id_var_printer print_var_;
    functor_printer_t print_functor_;
    printer_t printer_;
    steady_now clock_;
    readout_t readout_;
    timer_t solve_timer_;
    save_recorder_t save_recorder_;
    progress_t progress_;
    autosaver_t autosaver_;
    loop_t solve_loop_;
};

#endif
