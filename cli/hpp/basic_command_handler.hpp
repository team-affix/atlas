#ifndef BASIC_COMMAND_HANDLER_HPP
#define BASIC_COMMAND_HANDLER_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include "infrastructure/basic_solver_session.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/var_names.hpp"

struct basic_command_handler {
    basic_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        uint32_t seed);

    void operator()();

#ifdef DEBUG
    const db& test_database() const { return database_; }
    const initial_goal_exprs& test_initial_goals() const { return initial_goals_; }
    const std::map<std::string, uint32_t>& test_var_name_to_idx() const { return var_name_to_idx_; }
    uint32_t test_parse_var_peek() const { return parse_var_seq_.peek(); }
    void test_print_bindings() { print_bindings(); }
#endif

#ifndef DEBUG
private:
#endif
    void print_bindings();

    locator parse_loc_;
    trail parse_trail_;
    var_names var_names_;
    non_backtracking_var_sequencer parse_var_seq_;
    std::optional<expr_pool> parse_pool_;
    std::optional<expr_printer> printer_;
    db database_;
    initial_goal_exprs initial_goals_;
    std::map<std::string, uint32_t> var_name_to_idx_;
    std::optional<basic_solver_session> session_;
};

#endif
