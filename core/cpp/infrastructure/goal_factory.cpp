#include "../../hpp/infrastructure/goal_factory.hpp"
#include "../../hpp/domain/value_objects/goal.hpp"
#include "../../hpp/bootstrap/locator.hpp"

goal_factory::goal_factory()
    : candidate_factory_(locator::locate<i_factory<candidate, size_t>>()),
      lp_(locator::locate<i_lineage_pool>()),
      db_(locator::locate<i_database>()) {}

std::unique_ptr<goal> goal_factory::make(const goal_lineage* gl, const expr* e) {
    auto g = std::make_unique<goal>();
    g->e = e;

    for (size_t i = 0; i < db_.size(); ++i)
        g->candidates.emplace(i, candidate_factory_.make(i));

    return g;
}
