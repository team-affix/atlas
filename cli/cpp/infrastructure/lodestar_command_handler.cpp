#include "infrastructure/lodestar_command_handler.hpp"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include "infrastructure/hex_uint64.hpp"

lodestar_command_handler::lodestar_command_handler(
    const std::vector<std::string>& module_paths,
    const std::string& focus_text,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold,
    size_t max_node_depth,
    size_t max_open_goals,
    size_t sim_progress_interval,
    size_t unfolds_per_save,
    bool report_stored_solutions,
    bool quiet)
    : module_paths_(module_paths)
    , loaded_()
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
    , readout_(clock_, quiet)
    , solve_timer_(clock_)
    , save_recorder_(clock_, loaded_)
    , progress_(solve_timer_, runtime_, runtime_, runtime_, save_recorder_, isatty(fileno(stdout)) != 0)
    , autosaver_(runtime_, save_recorder_, runtime_, unfolds_per_save)
    , solve_loop_(
          runtime_, runtime_, runtime_, runtime_, runtime_, printer_, progress_, progress_, progress_, solve_timer_,
          solve_timer_, autosaver_, autosaver_, sim_progress_interval) {}

// Each module file is read and timed on its own; then the runtime is initialized by activating every
// module, dependencies first, which is the slow part on a large module.
void lodestar_command_handler::operator()() {
    hex_uint64 hex_codec;
    for (const std::string& path : module_paths_) {
        const pud_module_id module_id = loaded_.read_module_id(path);
        const readout_t::time_point read_started =
            readout_.begin("Loading module " + hex_codec.format_hex(module_id) + " (" + path + ")");
        loaded_.read(path);
        readout_.end("Loaded", read_started);
    }
    loaded_.sort_by_dependencies();
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
    const readout_t::time_point runtime_started = readout_.begin("Initializing runtime");
    for (pud_module& module : loaded_.modules())
        runtime_.register_module(module);
    readout_.end("Runtime initialized", runtime_started);
    solve_loop_.run();
}
