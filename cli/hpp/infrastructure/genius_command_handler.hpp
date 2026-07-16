#ifndef GENIUS_COMMAND_HANDLER_HPP
#define GENIUS_COMMAND_HANDLER_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/genius_runtime.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"
#include "infrastructure/print_bindings.hpp"
#include "infrastructure/genius_print_progress.hpp"
#include "infrastructure/print_progress.hpp"
#include "infrastructure/solve_loop.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/var_names.hpp"

struct genius_command_handler {
    using PrintBindings = print_bindings<genius_runtime, expr_printer>;
    using BasePP        = print_progress<genius_runtime>;
    using PP            = genius_print_progress<BasePP>;
    using SolveLoop     = solve_loop<genius_runtime, expr_printer, PrintBindings, PP>;

    genius_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        uint32_t seed,
        double exploration_constant = 1.414,
        size_t sim_progress_interval = 1000);

    void operator()();

private:
    var_names var_names_;
    functor_names functor_names_;
    non_backtracking_var_sequencer parse_var_seq_{0};
    std::map<std::string, uint32_t> functor_map_;
    uint32_t next_functor_id_ = k_first_user_functor_id;
    std::optional<expr_pool> parse_pool_;
    std::optional<expr_printer> printer_;
    db database_;
    initial_goal_exprs initial_goals_;
    std::map<std::string, uint32_t> var_name_to_idx_;
    std::optional<genius_runtime> runtime_;
    PrintBindings print_bindings_;
    BasePP        base_print_progress_;
    PP            print_progress_;
    SolveLoop     solve_loop_;
};

#endif
