#ifndef PUD_COMMAND_HANDLER_HPP
#define PUD_COMMAND_HANDLER_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/named_functor_printer.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "infrastructure/var_names.hpp"

// Loads a CHC database as PUD axioms, adds one conjecture from goal strings, and prints answers.
struct pud_command_handler {
    using printer_t = expr_printer<id_var_printer, named_functor_printer<functor_names>>;

    pud_command_handler(
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
    pud_manifest manifest_;
    db database_;
    pud_node_id conjecture_;
    size_t max_unfolds_;
    size_t max_answers_;
    std::optional<printer_t> printer_;
};

#endif
