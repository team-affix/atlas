#include "../../../hpp/domain/entities/active_eliminator.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

active_eliminator::active_eliminator()
    : gcs(resolver::resolve<i_goal_candidates_store>()),
    goal_candidates_changed_producer(resolver::resolve<i_event_producer<goal_candidates_changed_event>>()) {
}

void active_eliminator::eliminate(const goal_lineage* gl, size_t idx) {
    gcs.eliminate(gl, idx);
    goal_candidates_changed_producer.produce(goal_candidates_changed_event{gl});
}
