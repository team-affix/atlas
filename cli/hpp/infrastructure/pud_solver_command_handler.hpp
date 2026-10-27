#ifndef PUD_SOLVER_COMMAND_HANDLER_HPP
#define PUD_SOLVER_COMMAND_HANDLER_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/named_functor_printer.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "infrastructure/var_names.hpp"

// Loads a CHC database as one PUD module, queries it with goal strings, and prints answers.
struct pud_solver_command_handler {
    using printer_t = expr_printer<id_var_printer, named_functor_printer<functor_names>>;

    pud_solver_command_handler(
        const std::string& file,
        const std::string& goals_str,
        const std::string& head_str,
        size_t max_unfolds,
        size_t max_answers,
        uint32_t seed);

    void operator()();

private:
    var_names var_names_;
    id_var_printer print_var_;
    functor_names functor_names_;
    named_functor_printer<functor_names> print_functor_;
    non_backtracking_var_sequencer parse_var_seq_;
    std::map<std::string, uint32_t> functor_map_;
    uint32_t next_functor_id_;
    expr_pool expr_pool_;
    db database_;
    std::optional<pud_module> database_module_;
    pud_runtime<pud_module, expr_pool> runtime_;
    size_t max_unfolds_;
    size_t max_answers_;
    std::optional<printer_t> printer_;
};

#endif
