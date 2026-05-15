#include <algorithm>
#include "../../hpp/infrastructure/goal_initializer.hpp"
#include "../../hpp/bootstrap/locator.hpp"

goal_initializer::goal_initializer() :
    db_(locator::locate<i_database>()),
    initial_goal_exprs_(locator::locate<i_initial_goal_exprs>()),
    frontier_(locator::locate<i_frontier>()),
    copier_(locator::locate<i_copier>()) {
}

void goal_initializer::seed_expansion(const resolution_lineage* rl) {
    // if these are initial goals, just use the initial goal exprs
    if (rl == nullptr) {
        for (int i = 0; i < initial_goal_exprs_.size(); i++)
            child_goal_exprs_.push_back(initial_goal_exprs_.at(i));
        return;
    }

    // get the parent goal
    auto& g = frontier_.at(rl->parent);

    // get the tm and chosen rule body
    auto& tm = g->candidates.at(rl->idx)->tm;
    const auto& chosen_rule_body = db_.at(rl->idx).body;

    // get the child goal exprs
    child_goal_exprs_.clear();
    std::transform(
        chosen_rule_body.begin(),
        chosen_rule_body.end(),
        std::back_inserter(child_goal_exprs_),
        [this, &tm](const expr* e) {
        return copier_.copy(e, tm);
    });
}

void goal_initializer::initialize(const goal_lineage* lineage) {
    // get the goal
    auto& g = frontier_.at(lineage);

    g->e = child_goal_exprs_.at(lineage->idx);
}
