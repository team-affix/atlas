#include <unordered_set>
#include "../../hpp/infrastructure/goal_factory.hpp"
#include "../../hpp/domain/entities/goal.hpp"
#include "../../hpp/domain/entities/var_extractor.hpp"
#include "../../hpp/bootstrap/locator.hpp"

goal_factory::goal_factory()
    : candidate_factory_(locator::locate<i_candidate_factory>()),
      lp_(locator::locate<i_lineage_pool>()),
      db_(locator::locate<i_database>()),
      normalizer_(locator::locate<i_normalizer>()),
      traverser_factory_(locator::locate<i_expr_traverser_factory>()) {}

std::unique_ptr<i_goal> goal_factory::make(const goal_lineage* gl, const expr* e) {
    const expr* e_norm = normalizer_.normalize(e);

    std::unordered_set<const expr*> var_exprs;
    var_extractor ve(var_exprs);
    traverser_factory_.make(e_norm)->accept(ve);

    auto g = std::make_unique<goal>(e);

    for (const expr* v : var_exprs)
        g->e_reps().insert(std::get<expr::var>(v->content).index);

    for (size_t i = 0; i < db_.size(); ++i)
        g->candidates().insert(i, candidate_factory_.make(lp_.resolution(gl, i), e_norm));

    return g;
}
