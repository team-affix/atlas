#ifndef DBUCT_HORIZON_COMMAND_HANDLER_HPP
#define DBUCT_HORIZON_COMMAND_HANDLER_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"
#include "infrastructure/dbuct_horizon_runtime.hpp"
#include "infrastructure/pause_poller.hpp"
#include "infrastructure/print_bindings.hpp"
#include "infrastructure/horizon_print_progress.hpp"
#include "infrastructure/print_progress.hpp"
#include "infrastructure/solve_loop.hpp"
#include "infrastructure/solve_timer.hpp"
#include "infrastructure/stdin_idle.hpp"
#include "infrastructure/stdin_is_tty.hpp"
#include "infrastructure/stdin_try_read_byte.hpp"
#include "infrastructure/steady_now.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/var_names.hpp"

// CLI handler for the `dbuct-horizon` command: parses the database and goals, wires
// up a dbuct_horizon_runtime (camping DBUCT + horizon reward), and drives the shared
// solve loop that prints solutions and progress.
struct dbuct_horizon_command_handler {
    using printer_t = expr_printer<var_names, functor_names>;
    using SolveTimer     = solve_timer<steady_now>;
    using PrintBindings  = print_bindings<dbuct_horizon_runtime, printer_t>;
    using BasePP         = print_progress<dbuct_horizon_runtime, SolveTimer>;
    using PrintProgress  = horizon_print_progress<BasePP, dbuct_horizon_runtime>;
    using PausePoller    = pause_poller<PrintProgress, stdin_is_tty, stdin_try_read_byte, stdin_idle>;
    using SolveLoop      = solve_loop<dbuct_horizon_runtime, printer_t, PrintBindings, PrintProgress,
                                      PausePoller, PrintProgress, SolveTimer, SolveTimer>;

    dbuct_horizon_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        uint32_t seed,
        double exploration_constant = 2,
        double grant_k = dbuct_horizon_runtime::k_default_grant_k,
        size_t sim_progress_interval = 1000);

    void operator()();

private:
    var_names var_names_;
    functor_names functor_names_;
    non_backtracking_var_sequencer parse_var_seq_{0};
    std::map<std::string, uint32_t> functor_map_;
    uint32_t next_functor_id_ = k_first_user_functor_id;
    std::optional<expr_pool> parse_pool_;
    std::optional<printer_t> printer_;
    db database_;
    initial_goal_exprs initial_goals_;
    std::map<std::string, uint32_t> var_name_to_idx_;
    std::optional<dbuct_horizon_runtime> runtime_;
    steady_now clock_;
    SolveTimer solve_timer_;
    PrintBindings print_bindings_;
    BasePP        base_print_progress_;
    PrintProgress print_progress_;
    stdin_is_tty is_tty_;
    stdin_try_read_byte try_read_byte_;
    stdin_idle idle_;
    PausePoller pause_poller_;
    SolveLoop solve_loop_;
};

#endif
