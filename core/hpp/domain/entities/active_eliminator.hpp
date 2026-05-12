#ifndef ACTIVE_ELIMINATOR_HPP
#define ACTIVE_ELIMINATOR_HPP

#include "../interfaces/i_active_eliminator.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/candidate_eliminating_event.hpp"
#include "../events/candidate_eliminated_event.hpp"
#include "../events/eliminate_candidate_yield_event.hpp"

struct active_eliminator : i_active_eliminator {
    active_eliminator();
    void eliminate(const resolution_lineage*) override;
    void resume() override;
private:
    i_candidates_frontier& gcs;
    i_event_producer<candidate_eliminating_event>& candidate_eliminating_producer;
    i_event_producer<candidate_eliminated_event>& candidate_eliminated_producer;
    i_event_producer<eliminate_candidate_yield_event>& eliminate_candidate_yield_producer;

    const resolution_lineage* current_rl = nullptr;
};

#endif
