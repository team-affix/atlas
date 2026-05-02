#ifndef ACTIVE_ELIMINATOR_HPP
#define ACTIVE_ELIMINATOR_HPP

#include "../interfaces/i_active_eliminator.hpp"
#include "../interfaces/i_goal_candidates_store.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/goal_candidates_changed_event.hpp"

struct active_eliminator : i_active_eliminator {
    active_eliminator();
    void eliminate(const goal_lineage*, size_t) override;
private:
    i_goal_candidates_store& gcs;
    i_event_producer<goal_candidates_changed_event>& goal_candidates_changed_producer;
};

#endif
