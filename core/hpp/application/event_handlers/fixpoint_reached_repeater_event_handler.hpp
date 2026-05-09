#ifndef FIXPOINT_REACHED_REPEATER_EVENT_HANDLER_HPP
#define FIXPOINT_REACHED_REPEATER_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/fixpoint_reached_event.hpp"
#include "../../domain/events/sim_termination_condition_reached_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct fixpoint_reached_repeater_event_handler : cancellable_event_handler<fixpoint_reached_event, sim_termination_condition_reached_event, sim_started_event> {
    fixpoint_reached_repeater_event_handler();
    void execute(const fixpoint_reached_event&) override;
private:
    i_event_producer<fixpoint_reached_event>& fixpoint_reached_producer;
};

#endif
