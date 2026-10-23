#include "infrastructure/pud_command_handler.hpp"
#include "infrastructure/coroutine.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include "parser/hpp/import_goals_from_string.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

struct pud_goal_exprs {
    std::vector<const expr*> goals;
    void push(const expr* goal) { goals.push_back(goal); }
};

void load_axioms(db& database, pud_manifest& manifest) {
    std::vector<pud_node_id> roots;
    coroutine<rule_id, void> rules = database.lookup_all_rules().iterate();
    while (!rules.done()) {
        rules.resume();
        if (!rules.has_yield())
            break;
        const rule_id rule = rules.consume_yield();
        roots.push_back(manifest.node_maker_.make_root(*database.get_rule(rule), manifest.database_axiom_group_));
    }
    for (pud_node_id root : roots) {
        if (!manifest.root_activator_.activate_root(root))
            continue;
        manifest.leaf_refuter_.refute(root);
    }
}

} // namespace

pud_command_handler::pud_command_handler(
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
    , manifest_()
    , database_()
    , conjecture_(0)
    , max_unfolds_(max_unfolds)
    , max_answers_(max_answers)
    , printer_() {
    (void)goals_str;
    (void)head_str;
    (void)seed;
    import_database_from_file(
        file,
        manifest_.expr_pool_,
        manifest_.expr_pool_,
        database_,
        functor_map_,
        next_functor_id_);
    load_axioms(database_, manifest_);
}

void pud_command_handler::operator()() {
    std::cerr << "PUD automatic solving is not available yet (axioms loaded).\n";
}
