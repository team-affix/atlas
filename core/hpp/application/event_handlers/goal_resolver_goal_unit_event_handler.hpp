#ifndef GOAL_RESOLVER_GOAL_UNIT_EVENT_HANDLER_HPP
#define GOAL_RESOLVER_GOAL_UNIT_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/goal_unit_event.hpp"
#include "../../domain/events/sim_termination_condition_reached_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_goal_resolver.hpp"
#include "../../domain/interfaces/i_candidates_frontier.hpp"
#include "../../domain/interfaces/i_lineage_pool.hpp"

struct goal_resolver_goal_unit_event_handler : cancellable_event_handler<goal_unit_event, sim_termination_condition_reached_event, sim_started_event> {
    goal_resolver_goal_unit_event_handler();
    void execute(const goal_unit_event&) override;
private:
    i_goal_resolver& goal_resolver;
    i_candidates_frontier& candidates_frontier;
    i_lineage_pool& lp;
};

#endif
