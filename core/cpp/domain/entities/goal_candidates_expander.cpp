#include "../../../hpp/domain/entities/goal_candidates_expander.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"
#include "../../../hpp/domain/interfaces/i_database.hpp"

goal_candidates_expander::goal_candidates_expander()
    :
    gcs(resolver::resolve<i_candidates_frontier>()),
    goal_candidates_changed_producer(resolver::resolve<i_event_producer<goal_candidates_changed_event>>()) {
    i_database& db = resolver::resolve<i_database>();
    for (size_t i = 0; i < db.size(); ++i)
        initial_candidates.insert(i);
}

void goal_candidates_expander::start_expansion(const resolution_lineage* rl) {
}

void goal_candidates_expander::expand_child(const goal_lineage* gl) {
    gcs.insert(gl, candidate_set{initial_candidates});
    goal_candidates_changed_producer.produce(goal_candidates_changed_event{gl});
}
