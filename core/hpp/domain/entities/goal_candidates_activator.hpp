#ifndef GOAL_CANDIDATES_ACTIVATOR_HPP
#define GOAL_CANDIDATES_ACTIVATOR_HPP

#include <unordered_set>
#include "../interfaces/i_goal_candidates_activator.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/candidate_activated_event.hpp"

struct goal_candidates_activator : i_goal_candidates_activator {
    goal_candidates_activator();
    void start_resolution(const resolution_lineage*) override;
    void activate(const goal_lineage*) override;
private:
    i_candidates_frontier& cf;
    i_lineage_pool& lp;
    i_event_producer<candidate_activated_event>& candidate_activated_producer;
    std::unordered_set<size_t> all_candidates;
};

#endif
