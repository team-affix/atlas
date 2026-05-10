#include "../../../hpp/domain/entities/active_eliminator.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

active_eliminator::active_eliminator()
    : gcs(resolver::resolve<i_candidates_frontier>()),
    candidate_eliminated_producer(resolver::resolve<i_event_producer<candidate_eliminated_event>>()) {
}

void active_eliminator::eliminate(const resolution_lineage* rl) {
    gcs.at(rl->parent).candidates.erase(rl->idx);
    candidate_eliminated_producer.produce(candidate_eliminated_event{rl});
}
