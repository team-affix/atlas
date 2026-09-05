#include "infrastructure/unfold_command_handler.hpp"
#include "infrastructure/db_printer.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/named_functor_printer.hpp"
#include "infrastructure/named_var_printer.hpp"
#include "infrastructure/rule_printer.hpp"
#include "infrastructure/rule_unfolder_manifest.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

unfold_command_handler::unfold_command_handler(
    const std::string& basis,
    const std::string& derivation,
    rule_id subject_id,
    subgoal_id body_idx,
    bool print_db,
    bool overwrite)
    : var_names_()
    , functor_names_()
    , functor_map_()
    , next_functor_id_(k_first_user_functor_id)
    , parse_pool_()
    , original_db_()
    , expanded_db_()
    , derivation_path_(derivation)
    , subject_id_(subject_id)
    , body_idx_(body_idx)
    , print_db_(print_db)
    , overwrite_(overwrite) {
    const bool both_output_modes = print_db_ && overwrite_;
    if (both_output_modes)
        throw std::runtime_error("--print-db and --overwrite cannot be used together");
    import_database_from_file(
        basis, parse_pool_, parse_pool_, original_db_, functor_map_, next_functor_id_);
    import_database_from_file(
        derivation, parse_pool_, parse_pool_, expanded_db_, functor_map_, next_functor_id_);
    for (const auto& [name, id] : functor_map_)
        functor_names_.set_name(id, name);
}

void unfold_command_handler::print_mutated_db(std::ostream& os) {
    id_var_printer print_var;
    named_functor_printer<functor_names> print_functor(functor_names_);
    using print_expr_t = expr_printer<id_var_printer, named_functor_printer<functor_names>>;
    using print_rule_t = rule_printer<print_expr_t>;
    using print_db_t = db_printer<print_rule_t>;
    print_expr_t print_expr(os, print_var, print_functor);
    print_rule_t print_rule(os, print_expr);
    print_db_t print_db(os, print_rule);
    print_db.print(expanded_db_);
}

void unfold_command_handler::operator()() {
    const bool print_mutated = print_db_ || overwrite_;
    if (print_mutated) {
        rule_unfolder_manifest manifest(original_db_, expanded_db_);
        manifest.unfold(subject_id_, body_idx_);
        if (overwrite_) {
            std::ofstream out(derivation_path_);
            const bool open_failed = !out;
            if (open_failed)
                throw std::runtime_error(
                    "Failed to open derivation file for overwrite: " + derivation_path_);
            print_mutated_db(out);
            return;
        }
        print_mutated_db(std::cout);
        return;
    }
    named_var_printer<var_names> print_var(var_names_);
    named_functor_printer<functor_names> print_functor(functor_names_);
    using print_expr_t = expr_printer<named_var_printer<var_names>, named_functor_printer<functor_names>>;
    using print_rule_t = rule_printer<print_expr_t>;
    print_expr_t print_expr(std::cout, print_var, print_functor);
    print_rule_t print_rule(std::cout, print_expr);
    print_rule.print(*expanded_db_.get_rule(subject_id_));
    std::cout << "\n\n---\n\n";
    rule_unfolder_manifest manifest(original_db_, expanded_db_);
    const std::vector<rule_id> new_ids = manifest.unfold(subject_id_, body_idx_);
    for (rule_id id : new_ids) {
        print_rule.print(*expanded_db_.get_rule(id));
        std::cout << "\n\n";
    }
}
