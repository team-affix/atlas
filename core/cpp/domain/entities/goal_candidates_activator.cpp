#include "../../../hpp/domain/entities/goal_candidates_activator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/domain/interfaces/i_database.hpp"

goal_candidates_activator::goal_candidates_activator() :
    cf(locator::locate<i_candidates_frontier>()),
    lp(locator::locate<i_lineage_pool>()),
    candidate_activating_producer(locator::locate<i_event_producer<candidate_activating_event>>()),
    candidate_activated_producer(locator::locate<i_event_producer<candidate_activated_event>>()),
    activate_yielded_producer(locator::locate<i_event_producer<goal_candidates_activate_yielded_event>>()),
    db_size(locator::locate<i_database>().size()) {}

void goal_candidates_activator::init_activate(const goal_lineage* gl) {
    cf.insert(gl, candidate_set{});
    current_gl = gl;
    current_idx = 0;
    prev_rl = nullptr;
    activate_yielded_producer.produce({});
}

void goal_candidates_activator::resume() {
    if (prev_rl) candidate_activated_producer.produce({prev_rl});
    if (current_idx >= db_size) return;
    cf.at(current_gl).candidates.insert(current_idx);
    prev_rl = lp.resolution(current_gl, current_idx);
    candidate_activating_producer.produce({prev_rl});
    ++current_idx;
    activate_yielded_producer.produce({});
}
