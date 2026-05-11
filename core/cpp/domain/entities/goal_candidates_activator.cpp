#include "../../../hpp/domain/entities/goal_candidates_activator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/domain/interfaces/i_database.hpp"

goal_candidates_activator::goal_candidates_activator() :
    cf(locator::locate<i_candidates_frontier>()),
    lp(locator::locate<i_lineage_pool>()),
    candidate_activated_producer(locator::locate<i_event_producer<candidate_activated_event>>()) {
    i_database& db = locator::locate<i_database>();
    for (size_t i = 0; i < db.size(); ++i)
        all_candidates.insert(i);
}

void goal_candidates_activator::start_resolution(const resolution_lineage*) {}

void goal_candidates_activator::activate(const goal_lineage* gl) {
    cf.insert(gl, candidate_set{all_candidates});
    for (size_t rule_idx : all_candidates)
        candidate_activated_producer.produce(candidate_activated_event{lp.resolution(gl, rule_idx)});
}
