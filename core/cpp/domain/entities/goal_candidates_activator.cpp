#include "../../../hpp/domain/entities/goal_candidates_activator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/domain/interfaces/i_database.hpp"

goal_candidates_activator::goal_candidates_activator() :
    cf(locator::resolve<i_candidates_frontier>()),
    goal_candidates_changed_producer(locator::resolve<i_event_producer<goal_candidates_changed_event>>()) {
    i_database& db = locator::resolve<i_database>();
    for (size_t i = 0; i < db.size(); ++i)
        all_candidates.insert(i);
}

void goal_candidates_activator::start_resolution(const resolution_lineage*) {}

void goal_candidates_activator::activate(const goal_lineage* gl) {
    cf.insert(gl, candidate_set{all_candidates});
    goal_candidates_changed_producer.produce(goal_candidates_changed_event{gl});
}
