#ifndef RESOLVER_GOAL_UNIT_EVENT_HANDLER_HPP
#define RESOLVER_GOAL_UNIT_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/goal_unit_event.hpp"
#include "../../domain/events/sim_termination_condition_reached_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_resolver.hpp"
#include "../../domain/interfaces/i_frontier.hpp"
#include "../../domain/interfaces/i_lineage_pool.hpp"

struct resolver_goal_unit_event_handler : cancellable_event_handler<goal_unit_event, sim_termination_condition_reached_event, sim_started_event> {
    resolver_goal_unit_event_handler();
    void execute(const goal_unit_event&) override;
private:
    i_resolver& res;
    i_frontier& frontier;
    i_lineage_pool& lp;
};

#endif
