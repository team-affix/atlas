#ifndef GOAL_CANDIDATES_DEACTIVATOR_HPP
#define GOAL_CANDIDATES_DEACTIVATOR_HPP

#include "../interfaces/i_goal_candidates_deactivator.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/candidate_deactivating_event.hpp"

struct goal_candidates_deactivator : i_goal_candidates_deactivator {
    goal_candidates_deactivator();
    void deactivate(const goal_lineage*) override;
private:
    i_candidates_frontier& cf;
    i_lineage_pool& lp;
    i_event_producer<candidate_deactivating_event>& candidate_deactivating_producer;
};

#endif
