#include "infrastructure/pud_command_handler.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include "parser/hpp/import_goals_from_string.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_solution.hpp"
#include "value_objects/pud_unfold.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

struct pud_goal_exprs {
    std::vector<const expr*> goals;
    void push(const expr* goal) { goals.push_back(goal); }
};

void load_axioms(db& database, pud_manifest& manifest) {
    coroutine<rule_id, void> rules = database.lookup_all_rules().iterate();
    while (!rules.done()) {
        rules.resume();
        if (!rules.has_yield())
            break;
        const rule_id rule = rules.consume_yield();
        manifest.root_opener_.add_axiom(*database.get_rule(rule));
    }
    manifest.root_opener_.open();
}

const expr* parse_single_expr(
    const std::string& text,
    expr_pool& pool,
    non_backtracking_var_sequencer& var_seq,
    std::map<std::string, uint32_t>& functor_map,
    uint32_t& next_functor_id) {
    pud_goal_exprs parsed;
    import_goals_from_string(text, pool, pool, var_seq, parsed, functor_map, next_functor_id);
    if (parsed.goals.empty())
        throw std::runtime_error("expected at least one expression: " + text);
    return parsed.goals.front();
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
    , manifest_(seed)
    , database_()
    , conjecture_(0)
    , max_unfolds_(max_unfolds)
    , max_answers_(max_answers)
    , printer_() {
    import_database_from_file(
        file,
        manifest_.expr_pool_,
        manifest_.expr_pool_,
        database_,
        functor_map_,
        next_functor_id_);
    load_axioms(database_, manifest_);

    pud_goal_exprs goals;
    const std::map<std::string, uint32_t> goal_vars = import_goals_from_string(
        goals_str,
        manifest_.expr_pool_,
        manifest_.expr_pool_,
        parse_var_seq_,
        goals,
        functor_map_,
        next_functor_id_);
    if (goals.goals.empty())
        throw std::runtime_error("conjecture goals must not be empty");

    const expr* head = head_str.empty() ? goals.goals.front() : parse_single_expr(head_str, manifest_.expr_pool_, parse_var_seq_, functor_map_, next_functor_id_);
    const uint32_t var_count = parse_var_seq_.peek();
    for (const auto& [name, index] : goal_vars)
        var_names_.set_name(index, name);
    for (const auto& [name, id] : functor_map_)
        functor_names_.set_name(id, name);

    conjecture_ = manifest_.solver_.add_conjecture(head, goals.goals, var_count);
    printer_.emplace(std::cout, print_var_, print_functor_);
}

void pud_command_handler::operator()() {
    std::optional<coroutine<pud_solver_step, void>> solving;
    size_t unfolds = 0;
    size_t answers = 0;
    while (unfolds < max_unfolds_ && answers < max_answers_) {
        if (!solving.has_value() || solving->done())
            solving.emplace(manifest_.solver_.solve());
        solving->resume();
        if (!solving->has_yield())
            break;
        const pud_solver_step step = solving->consume_yield();
        if (const pud_solution* solution = std::get_if<pud_solution>(&step)) {
            printer_->print(solution->answer);
            std::cout << '\n';
            ++answers;
            continue;
        }
        ++unfolds;
    }
    if (manifest_.solver_.exhausted(conjecture_))
        std::cout << "exhausted\n";
}
