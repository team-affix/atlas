#ifndef SIM_STARTED_NO_MORE_UNIT_GOALS_BRIDGE_EVENT_HANDLER_HPP
#define SIM_STARTED_NO_MORE_UNIT_GOALS_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/events/no_more_unit_goals_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct sim_started_no_more_unit_goals_bridge_event_handler : event_handler<sim_started_event> {
    sim_started_no_more_unit_goals_bridge_event_handler();
    void handle(const sim_started_event&) override;
private:
    i_event_producer<no_more_unit_goals_event>& no_more_unit_goals_producer;
};

#endif
