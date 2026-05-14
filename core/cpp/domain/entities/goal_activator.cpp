#include "../../../hpp/domain/entities/goal_activator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/infrastructure/unordered_set.hpp"

goal_activator::goal_activator(std::vector<const expr*> initial_exprs)
    : goal_factory_(locator::locate<i_goal_factory>()),
      expr_traverser_factory_(locator::locate<i_factory<i_expr_traverser, const expr*>>()),
      var_id_extractor_factory_(locator::locate<i_factory<i_var_id_extractor, i_set<uint32_t>&>>()),
      candidate_factory_(locator::locate<i_factory<candidate>>()),
      frontier_(locator::locate<i_frontier>()),
      db_(locator::locate<i_database>()),
      copier_(locator::locate<i_copier>()),
      unifier_(locator::locate<i_unifier>()),
      normalizer_(locator::locate<i_normalizer>()),
      initial_exprs_(std::move(initial_exprs)) {}

void goal_activator::start_resolution(const resolution_lineage* rl) {
    if (rl == nullptr) {
        current_exprs_ = initial_exprs_;
        return;
    }

    const candidate& c = *frontier_.at(rl->parent)->candidates->at(rl->idx);
    const rule& r = db_.at(rl->idx);

    const expr* parent_expr = frontier_.at(rl->parent)->e;
    unifier_.push(parent_expr, c.copied_head);

    current_exprs_.clear();
    for (const expr* e : r.body)
        current_exprs_.push_back(copier_.copy(e, *c.tm));
}

void goal_activator::activate(const goal_lineage* gl) {
    frontier_.at(gl)->e = current_exprs_[gl->idx];
    
    // init reprs
    const expr* norm_e = normalizer_.normalize(frontier_.at(gl)->e);
    auto traverser = expr_traverser_factory_.make(norm_e);
    auto& var_set = *frontier_.at(gl)->e_reps;
    auto var_extractor = var_id_extractor_factory_.make(var_set);
    traverser->accept(*var_extractor);

    // insert candidate
    frontier_.at(gl)->candidates->insert(candidate_factory_.make());
}
