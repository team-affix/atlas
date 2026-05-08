#ifndef GOAL_RESOLVER_DECIDED_EVENT_HANDLER_HPP
#define GOAL_RESOLVER_DECIDED_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/decided_event.hpp"
#include "../../domain/events/sim_cancelled_event.hpp"
#include "../../domain/events/sim_cancellation_reset_event.hpp"
#include "../../domain/interfaces/i_goal_resolver.hpp"

struct goal_resolver_decided_event_handler : cancellable_event_handler<decided_event, sim_cancelled_event, sim_cancellation_reset_event> {
    goal_resolver_decided_event_handler();
    void execute(const decided_event&) override;
private:
    i_goal_resolver& goal_resolver;
};

#endif
