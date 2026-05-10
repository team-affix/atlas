#include "../../../hpp/domain/entities/goal_weight_activator.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_weight_activator::goal_weight_activator(size_t initial_goal_count) :
    wf(resolver::resolve<i_weight_frontier>()),
    db(resolver::resolve<i_database>()),
    initial_weight(1.0 / initial_goal_count) {}

void goal_weight_activator::start_resolution(const resolution_lineage* rl) {
    if (rl == nullptr) {
        current_weight = initial_weight;
        return;
    }

    current_weight = wf.at(rl->parent) / db.at(rl->idx).body.size();
}

void goal_weight_activator::activate(const goal_lineage* gl) {
    wf.insert(gl, current_weight);
}
