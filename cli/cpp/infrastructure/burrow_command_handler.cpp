#include "infrastructure/burrow_command_handler.hpp"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>
#include "infrastructure/hex_uint64.hpp"

burrow_command_handler::burrow_command_handler(
    const std::vector<std::string>& module_directories,
    const std::string& focus_text,
    size_t max_unfolds,
    size_t cascade_depth,
    size_t max_node_depth,
    size_t max_open_goals,
    size_t state_cache_bytes,
    size_t node_cache_bytes,
    size_t max_disk_bytes,
    size_t sim_progress_interval,
    size_t unfolds_per_save,
    bool report_stored_solutions,
    bool quiet)
    : module_directories_(module_directories)
    , quiet_(quiet)
    , opened_(node_cache_bytes)
    , focus_(pud_focus_parser{}.parse(focus_text))
    , runtime_(
          opened_.exprs(),
          focus_,
          max_unfolds,
          cascade_depth,
          max_node_depth,
          max_open_goals,
          state_cache_bytes,
          max_disk_bytes,
          report_stored_solutions)
    , print_var_()
    , print_functor_(runtime_)
    , printer_(std::cout, print_var_, print_functor_)
    , clock_()
    , readout_(clock_, quiet)
    , solve_timer_(clock_)
    , save_recorder_(clock_, opened_)
    , progress_(solve_timer_, runtime_, runtime_, runtime_, save_recorder_, isatty(fileno(stdout)) != 0)
    , autosaver_(runtime_, save_recorder_, runtime_, unfolds_per_save)
    , solve_loop_(
          runtime_, runtime_, runtime_, runtime_, runtime_, printer_, progress_, progress_, progress_, solve_timer_,
          solve_timer_, autosaver_, autosaver_, sim_progress_interval) {}

// Opening a directory reads its header only, and registering a module walks just the focus tree's ids, so the
// runtime is ready at once whatever the modules' size. The cache totals follow the final line unless quiet.
void burrow_command_handler::operator()() {
    for (const std::string& directory : module_directories_) {
        const readout_t::time_point opened = readout_.begin("Opening module " + directory);
        opened_.open(directory);
        readout_.end("Opened", opened);
    }
    const std::vector<pud_paged_module_assembly*> ordered = opened_.in_dependency_order();
    bool focus_loaded = false;
    for (pud_paged_module_assembly* module : ordered) {
        if (module->module().get_module_id() != focus_.module)
            continue;
        if (focus_.node >= module->module().count_nodes())
            throw std::runtime_error("focus node is outside the module");
        focus_loaded = true;
    }
    if (!focus_loaded)
        throw std::runtime_error("focus module was not loaded");
    const readout_t::time_point runtime_started = readout_.begin("Initializing runtime");
    for (pud_paged_module_assembly* module : ordered)
        runtime_.register_module(*module);
    readout_.end("Runtime initialized", runtime_started);
    solve_loop_.run();
    if (quiet_)
        return;
    size_t node_hits = 0;
    size_t node_misses = 0;
    for (pud_paged_module_assembly* module : ordered) {
        node_hits += module->cache_hits();
        node_misses += module->cache_misses();
    }
    std::cerr << "cache: state hits " << runtime_.state_cache_hits() << " misses " << runtime_.state_cache_misses()
              << " | node hits " << node_hits << " misses " << node_misses << " | disk "
              << runtime_.disk_bytes() / (1024 * 1024) << " MB\n";
}
