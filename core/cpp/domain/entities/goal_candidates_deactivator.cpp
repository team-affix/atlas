#include "../../../hpp/domain/entities/goal_candidates_deactivator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_deactivator::goal_candidates_deactivator() :
    cf(locator::locate<i_candidates_frontier>()),
    lp(locator::locate<i_lineage_pool>()),
    candidate_deactivating_producer(locator::locate<i_event_producer<candidate_deactivating_event>>()),
    candidate_deactivated_producer(locator::locate<i_event_producer<candidate_deactivated_event>>()),
    deactivate_yielded_producer(locator::locate<i_event_producer<goal_candidates_deactivate_yielded_event>>()) {}

void goal_candidates_deactivator::init_deactivate(const goal_lineage* gl) {
    current_gl = gl;
    prev_rl = nullptr;
    deactivate_yielded_producer.produce({});
}

void goal_candidates_deactivator::resume() {
    if (prev_rl) candidate_deactivated_producer.produce({prev_rl});
    auto& candidates = cf.at(current_gl).candidates;
    if (candidates.empty()) {
        cf.erase(current_gl);
        return;
    }
    auto it = candidates.begin();
    size_t rule_idx = *it;
    candidates.erase(it);
    prev_rl = lp.resolution(current_gl, rule_idx);
    candidate_deactivating_producer.produce({prev_rl});
    deactivate_yielded_producer.produce({});
}
