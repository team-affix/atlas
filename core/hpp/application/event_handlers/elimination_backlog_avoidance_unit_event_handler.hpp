#ifndef ELIMINATION_BACKLOG_AVOIDANCE_UNIT_EVENT_HANDLER_HPP
#define ELIMINATION_BACKLOG_AVOIDANCE_UNIT_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/avoidance_unit_event.hpp"
#include "../../domain/events/sim_termination_condition_reached_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_cdcl.hpp"
#include "../../domain/interfaces/i_elimination_backlog.hpp"

struct elimination_backlog_avoidance_unit_event_handler : cancellable_event_handler<avoidance_unit_event, sim_termination_condition_reached_event, sim_started_event> {
    elimination_backlog_avoidance_unit_event_handler();
    void execute(const avoidance_unit_event&) override;
private:
    i_cdcl& c;
    i_elimination_backlog& elimination_backlog;
};

#endif
