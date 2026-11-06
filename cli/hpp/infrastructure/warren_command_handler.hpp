#ifndef WARREN_COMMAND_HANDLER_HPP
#define WARREN_COMMAND_HANDLER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/global_functor_name_printer.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/pud_warren_runtime.hpp"
#include "infrastructure/pud_focus_parser.hpp"
#include "infrastructure/pud_load_readout.hpp"
#include "infrastructure/pud_module_autosaver.hpp"
#include "infrastructure/pud_opened_paged_modules.hpp"
#include "infrastructure/pud_save_time_recorder.hpp"
#include "infrastructure/pud_solve_loop.hpp"
#include "infrastructure/pud_unfold_progress.hpp"
#include "infrastructure/solve_timer.hpp"
#include "infrastructure/steady_now.hpp"
#include "value_objects/pud_global_node_id.hpp"

// `atlas warren`: runs Warren (Burrow with Cairn's trails) over paged module directories. Opening a directory reads only its header; the run
// writes its unfolds straight to the files, and saving flushes them.
struct warren_command_handler {
    warren_command_handler(
        const std::vector<std::string>& module_directories,
        const std::string& focus_text,
        size_t max_unfolds,
        size_t cascade_depth,
        size_t max_node_depth,
        size_t max_open_goals,
        size_t state_cache_bytes,
        size_t trail_cache_bytes,
        size_t node_cache_bytes,
        size_t max_disk_bytes,
        size_t sim_progress_interval,
        size_t unfolds_per_save,
        bool report_stored_solutions,
        bool quiet);
    void operator()();
private:
    using functor_printer_t = global_functor_name_printer<pud_warren_runtime>;
    using printer_t = expr_printer<id_var_printer, functor_printer_t>;
    using readout_t = pud_load_readout<steady_now>;
    using timer_t = solve_timer<steady_now>;
    using save_recorder_t = pud_save_time_recorder<steady_now, pud_opened_paged_modules>;
    using progress_t = pud_unfold_progress<
        timer_t, pud_warren_runtime, pud_warren_runtime, pud_warren_runtime, save_recorder_t>;
    using autosaver_t = pud_module_autosaver<pud_warren_runtime, save_recorder_t, pud_warren_runtime>;
    using loop_t = pud_solve_loop<
        pud_warren_runtime, pud_warren_runtime, pud_warren_runtime, pud_warren_runtime, pud_warren_runtime,
        printer_t, progress_t, progress_t, progress_t, timer_t, timer_t, autosaver_t, autosaver_t>;

    std::vector<std::string> module_directories_;
    bool quiet_;
    pud_opened_paged_modules opened_;
    pud_global_node_id focus_;
    pud_warren_runtime runtime_;
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
