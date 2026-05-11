#ifndef GOAL_CANDIDATES_DEACTIVATOR_HPP
#define GOAL_CANDIDATES_DEACTIVATOR_HPP

#include "../interfaces/i_goal_candidates_deactivator.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/candidate_deactivating_event.hpp"
#include "../events/candidate_deactivated_event.hpp"
#include "../events/goal_candidates_deactivate_yielded_event.hpp"

struct goal_candidates_deactivator : i_goal_candidates_deactivator {
    goal_candidates_deactivator();
    void init_deactivate(const goal_lineage*) override;
    void resume() override;
private:
    i_candidates_frontier& cf;
    i_lineage_pool& lp;
    i_event_producer<candidate_deactivating_event>& candidate_deactivating_producer;
    i_event_producer<candidate_deactivated_event>& candidate_deactivated_producer;
    i_event_producer<goal_candidates_deactivate_yielded_event>& deactivate_yielded_producer;

    const goal_lineage* current_gl = nullptr;
    const resolution_lineage* prev_rl = nullptr;
};

#endif
