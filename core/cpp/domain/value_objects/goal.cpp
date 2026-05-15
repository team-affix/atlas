#include "../../../hpp/domain/value_objects/goal.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/domain/interfaces/i_copier.hpp"
#include "../../../hpp/domain/interfaces/i_frontier.hpp"
#include "../../../hpp/domain/interfaces/i_database.hpp"
#include "../../../hpp/domain/interfaces/i_initial_goal_exprs.hpp"

goal::goal(const goal_lineage* lineage) :
    db_(locator::locate<i_database>()),
    chosen_rule_body(nullptr),
    chosen_rule_tm(nullptr) {
    auto rl = lineage->parent;
    if (rl == nullptr) {
        const auto& ig = locator::locate<i_initial_goal_exprs>();
        e = ig.at(lineage->idx);
    } else {
        const auto& f = locator::locate<i_frontier>();
        const auto& c = locator::locate<i_copier>();
        auto parent = rl->parent;
        const auto& parent_g = f.at(parent);
        e = c.copy(
            parent_g->chosen_rule_body->at(lineage->idx),
            *parent_g->chosen_rule_tm);
    }
}

void goal::choose(size_t idx) {
    chosen_rule_body = &db_.at(idx).body;
    chosen_rule_tm = &candidates.at(idx)->tm;
}
