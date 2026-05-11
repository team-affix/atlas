#ifndef GOAL_CANDIDATES_ACTIVATOR_HPP
#define GOAL_CANDIDATES_ACTIVATOR_HPP

#include "../interfaces/i_goal_candidates_activator.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/candidate_activating_event.hpp"
#include "../events/candidate_activated_event.hpp"
#include "../events/goal_candidates_activate_yielded_event.hpp"

struct goal_candidates_activator : i_goal_candidates_activator {
    goal_candidates_activator();
    void init_activate(const goal_lineage*) override;
    void resume() override;
private:
    i_candidates_frontier& cf;
    i_lineage_pool& lp;
    i_event_producer<candidate_activating_event>& candidate_activating_producer;
    i_event_producer<candidate_activated_event>& candidate_activated_producer;
    i_event_producer<goal_candidates_activate_yielded_event>& activate_yielded_producer;

    size_t db_size;
    const goal_lineage* current_gl = nullptr;
    size_t current_idx = 0;
    const resolution_lineage* prev_rl = nullptr;
};

#endif
