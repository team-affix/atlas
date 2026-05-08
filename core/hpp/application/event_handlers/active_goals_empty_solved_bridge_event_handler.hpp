#ifndef ACTIVE_GOALS_EMPTY_SOLVED_BRIDGE_EVENT_HANDLER_HPP
#define ACTIVE_GOALS_EMPTY_SOLVED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/active_goals_empty_event.hpp"
#include "../../domain/events/solved_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct active_goals_empty_solved_bridge_event_handler : event_handler<active_goals_empty_event> {
    active_goals_empty_solved_bridge_event_handler();
    void handle(const active_goals_empty_event&) override;
private:
    i_event_producer<solved_event>& solved_producer;
};

#endif
