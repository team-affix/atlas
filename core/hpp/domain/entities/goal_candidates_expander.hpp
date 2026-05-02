#ifndef GOAL_CANDIDATES_EXPANDER_HPP
#define GOAL_CANDIDATES_EXPANDER_HPP

#include "../interfaces/i_goal_candidates_expander.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_goal_candidates_store.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/goal_candidates_changed_event.hpp"

struct goal_candidates_expander : i_goal_candidates_expander {
    goal_candidates_expander();
    void start_expansion(const resolution_lineage*) override;
    void expand_child(const goal_lineage*) override;
private:
    i_database& db;
    i_goal_candidates_store& gcs;
    i_event_producer<goal_candidates_changed_event>& goal_candidates_changed_producer;

    std::unordered_set<size_t> initial_candidates;
};

#endif
