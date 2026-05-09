#ifndef ROUTER_AVOIDANCE_UNIT_EVENT_HANDLER_HPP
#define ROUTER_AVOIDANCE_UNIT_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/avoidance_unit_event.hpp"
#include "../../domain/events/sim_termination_condition_reached_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_cdcl.hpp"
#include "../../domain/interfaces/i_active_goal_store.hpp"
#include "../../domain/interfaces/i_inactive_goal_store.hpp"
#include "../../domain/interfaces/i_elimination_backlog.hpp"
#include "../../domain/interfaces/i_active_eliminator.hpp"

struct router_avoidance_unit_event_handler : cancellable_event_handler<avoidance_unit_event, sim_termination_condition_reached_event, sim_started_event> {
    router_avoidance_unit_event_handler();
    void execute(const avoidance_unit_event&) override;
private:
    i_cdcl& c;
    i_active_goal_store& active_goal_store;
    i_inactive_goal_store& inactive_goal_store;
    i_elimination_backlog& elimination_backlog;
    i_active_eliminator& active_eliminator;
};

#endif
