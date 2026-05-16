#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include <optional>
#include "../interfaces/i_resolver.hpp"
#include "../../utility/state_machine.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../interfaces/i_frontier.hpp"
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_multihead_unifier.hpp"
#include "../interfaces/i_factory.hpp"
#include "../interfaces/i_goal_initializer.hpp"
#include "../interfaces/i_candidate_initializer.hpp"
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
    state_machine<void> resolve(const resolution_lineage*, size_t);
    state_machine<void> activate_goals(const resolution_lineage*, size_t);
    state_machine<void> activate_candidates(const goal_lineage*, goal&);
    state_machine<void> deactivate_goal(const goal_lineage*);
    state_machine<void> deactivate_candidates(const goal_lineage*, goal&);

    i_database& db;
    i_lineage_pool& lp;
    i_frontier& frontier;
    i_cdcl& c;
    i_multihead_unifier& mhu;
    i_factory<goal>& goal_factory;
    i_factory<candidate>& candidate_factory;
    i_goal_initializer& goal_initializer;
    i_candidate_initializer& candidate_initializer;
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
    
    // const resolution_lineage* parent_rl;
    // size_t body_size = 0;
    
    // bool activating_candidates = false;
    // const resolution_lineage* current_rl = nullptr;
    // size_t candidate_idx = 0;

    // bool activating_subgoals = false;
    // const goal_lineage* current_gl = nullptr;
    // size_t subgoal_idx = 0;

    // bool deactivating_candidates = false;

    // bool deactivating_goal = false;

    // bool finishing = false;

    std::optional<state_machine<void>> resolve_state_machine;
};

#endif
