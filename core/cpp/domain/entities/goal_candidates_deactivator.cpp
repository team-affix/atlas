#include "../../../hpp/domain/entities/goal_candidates_deactivator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_deactivator::goal_candidates_deactivator() :
    cf(locator::locate<i_candidates_frontier>()),
    lp(locator::locate<i_lineage_pool>()),
    candidate_deactivating_producer(locator::locate<i_event_producer<candidate_deactivating_event>>()) {}

void goal_candidates_deactivator::deactivate(const goal_lineage* gl) {
    const candidate_set& candidates = cf.at(gl);
    for (size_t rule_idx : candidates.candidates)
        candidate_deactivating_producer.produce(candidate_deactivating_event{lp.resolution(gl, rule_idx)});
    cf.erase(gl);
}
