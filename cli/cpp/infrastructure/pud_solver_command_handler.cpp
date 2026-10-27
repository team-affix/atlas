#include "infrastructure/pud_solver_command_handler.hpp"
#include "infrastructure/coroutine.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include "parser/hpp/import_goals_from_string.hpp"
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

std::vector<rule> list_rules(db& database) {
    std::vector<rule> rules;
    coroutine<rule_id, void> listed = database.lookup_all_rules().iterate();
    while (!listed.done()) {
        listed.resume();
        if (!listed.has_yield())
            break;
        rules.push_back(*database.get_rule(listed.consume_yield()));
    }
    return rules;
}

}

pud_solver_command_handler::pud_solver_command_handler(
    const std::string& file,
    const std::string& goals_str,
    const std::string& head_str,
    size_t max_unfolds,
    size_t max_answers,
    uint32_t seed)
    : print_var_()
    , print_functor_(functor_names_)
    , parse_var_seq_(0)
    , next_functor_id_(k_first_user_functor_id)
    , expr_pool_()
    , database_()
    , database_module_()
    , runtime_(expr_pool_)
    , max_unfolds_(max_unfolds)
    , max_answers_(max_answers)
    , printer_() {
    (void)goals_str;
    (void)head_str;
    import_database_from_file(
        file,
        expr_pool_,
        expr_pool_,
        database_,
        functor_map_,
        next_functor_id_);
    const pud_module_id module_id = std::mt19937_64{seed}();
    database_module_.emplace(module_id, std::vector<pud_module_id>{}, list_rules(database_));
    runtime_.register_module(database_module_.value());
}

void pud_solver_command_handler::operator()() {
    std::cerr << "PUD automatic solving is not available yet (axioms loaded).\n";
}
