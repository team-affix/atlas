#ifndef GOAL_CANDIDATES_ACTIVATOR_HPP
#define GOAL_CANDIDATES_ACTIVATOR_HPP

#include <unordered_set>
#include "../interfaces/i_goal_candidates_activator.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/goal_candidates_changed_event.hpp"

struct goal_candidates_activator : i_goal_candidates_activator {
    goal_candidates_activator();
    void start_resolution(const resolution_lineage*) override;
    void activate(const goal_lineage*) override;
private:
    i_candidates_frontier& cf;
    i_event_producer<goal_candidates_changed_event>& goal_candidates_changed_producer;
    std::unordered_set<size_t> all_candidates;
};

#endif
