#ifndef HORIZON_COMMAND_HANDLER_HPP
#define HORIZON_COMMAND_HANDLER_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/horizon_runtime.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"
#include "infrastructure/print_bindings.hpp"
#include "infrastructure/print_progress.hpp"
#include "infrastructure/solve_loop.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/var_names.hpp"

struct horizon_command_handler {
    horizon_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        uint32_t seed,
        double exploration_constant = 1.414,
        size_t sim_progress_interval = 1000);

    void operator()();

private:
    locator parse_loc_;
    trail parse_trail_;
    var_names var_names_;
    non_backtracking_var_sequencer parse_var_seq_;
    std::optional<expr_pool> parse_pool_;
    std::optional<expr_printer> printer_;
    db database_;
    initial_goal_exprs initial_goals_;
    std::map<std::string, uint32_t> var_name_to_idx_;
    std::optional<horizon_runtime> runtime_;
    print_bindings print_bindings_;
    print_progress print_progress_;
    solve_loop solve_loop_;
};

#endif
