#ifndef DBUCT_RIDGE_COMMAND_HANDLER_HPP
#define DBUCT_RIDGE_COMMAND_HANDLER_HPP

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
#include "infrastructure/dbuct_runtime.hpp"
#include "infrastructure/print_bindings.hpp"
#include "infrastructure/print_progress.hpp"
#include "infrastructure/solve_loop.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/var_names.hpp"

// CLI handler for the `dbuct-ridge` command: parses the database and goals, wires
// up a dbuct_runtime (the delayed-backtracking solver), and drives the shared
// solve loop that prints solutions and progress.
struct dbuct_ridge_command_handler {
    using PrintBindings  = print_bindings<dbuct_runtime, expr_printer>;
    using PrintProgress  = print_progress<dbuct_runtime>;
    using SolveLoop      = solve_loop<dbuct_runtime, expr_printer, PrintBindings, PrintProgress>;

    dbuct_ridge_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        uint32_t seed,
        double exploration_constant = 1.414,
        size_t grant_increment_interval = dbuct_runtime::kDefaultGrantIncrementInterval,
        size_t sim_progress_interval = 1000);

    void operator()();

private:
    var_names var_names_;
    functor_names functor_names_;
    non_backtracking_var_sequencer parse_var_seq_;
    std::map<std::string, uint32_t> functor_map_;
    uint32_t next_functor_id_ = k_first_user_functor_id;
    std::optional<expr_pool> parse_pool_;
    std::optional<expr_printer> printer_;
    db database_;
    initial_goal_exprs initial_goals_;
    std::map<std::string, uint32_t> var_name_to_idx_;
    std::optional<dbuct_runtime> runtime_;
    PrintBindings print_bindings_;
    PrintProgress print_progress_;
    SolveLoop solve_loop_;
};

#endif
