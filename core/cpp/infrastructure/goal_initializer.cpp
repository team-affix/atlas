#include "../../hpp/infrastructure/goal_initializer.hpp"
#include "../../hpp/bootstrap/locator.hpp"

goal_initializer::goal_initializer() :
    db_(locator::locate<i_database>()),
    initial_goal_exprs_(locator::locate<i_initial_goal_exprs>()),
    frontier_(locator::locate<i_frontier>()),
    copier_(locator::locate<i_copier>()) {
}

void goal_initializer::seed_expansion(const resolution_lineage* rl) {
    if (rl == nullptr)
        return;
    auto& g = frontier_.at(rl->parent);
    tm_ = &g->candidates.at(rl->idx)->tm;
    chosen_rule_body_ = &db_.at(rl->idx).body;
}

void goal_initializer::initialize(const goal_lineage* lineage) {
    // get the goal
    auto& g = frontier_.at(lineage);

    g->e = lineage->parent == nullptr
        ? initial_goal_exprs_.at(lineage->idx)
        : copier_.copy(
            chosen_rule_body_->at(lineage->idx),
            *tm_);
}
