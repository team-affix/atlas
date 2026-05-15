#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "../interfaces/i_resolver.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/resolving_event.hpp"
#include "../events/resolved_event.hpp"
#include "../events/goal_activating_event.hpp"
#include "../events/goal_activated_event.hpp"
#include "../events/goal_deactivating_event.hpp"
#include "../events/goal_deactivated_event.hpp"
#include "../events/resolve_yielded_event.hpp"
#include "../events/candidate_activating_event.hpp"
#include "../events/candidate_activated_event.hpp"
#include "../events/candidate_deactivating_event.hpp"
#include "../events/candidate_deactivated_event.hpp"

struct resolver : i_resolver {
    explicit resolver(size_t initial_goal_count);
    void init_resolve(const resolution_lineage*) override;
    void resume() override;
private:
    void resume_activating_subgoals();
    void resume_activating_candidates();
    void resume_deactivating_goal();
    void resume_deactivating_candidates();
    void finish();
    void start_activating_subgoals();
    void start_activating_candidates();
    void start_deactivating_goal();
    void start_deactivating_candidates();
    void start_finishing();

    i_database& db;
    i_lineage_pool& lp;
    i_event_producer<resolving_event>& resolving_producer;
    i_event_producer<resolved_event>& resolved_producer;
    i_event_producer<goal_activating_event>& goal_activating_producer;
    i_event_producer<goal_activated_event>& goal_activated_producer;
    i_event_producer<goal_deactivating_event>& goal_deactivating_producer;
    i_event_producer<goal_deactivated_event>& goal_deactivated_producer;
    i_event_producer<resolve_yielded_event>& resolve_yielded_producer;
    i_event_producer<candidate_activating_event>& candidate_activating_producer;
    i_event_producer<candidate_activated_event>& candidate_activated_producer;
    i_event_producer<candidate_deactivating_event>& candidate_deactivating_producer;
    i_event_producer<candidate_deactivated_event>& candidate_deactivated_producer;

    size_t initial_goal_count;
    
    const resolution_lineage* parent_rl = nullptr;
    size_t body_size = 0;
    
    bool activating_candidates = false;
    const resolution_lineage* current_rl = nullptr;
    size_t candidate_idx = 0;

    bool activating_subgoals = false;
    const goal_lineage* current_gl = nullptr;
    size_t subgoal_idx = 0;

    bool deactivating_candidates = false;

    bool deactivating_goal = false;

    bool finishing = false;
};

#endif
