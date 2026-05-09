#ifndef DECIDER_FIXPOINT_REACHED_EVENT_HANDLER_HPP
#define DECIDER_FIXPOINT_REACHED_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/fixpoint_reached_event.hpp"
#include "../../domain/events/sim_termination_condition_reached_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_decider.hpp"

struct decider_fixpoint_reached_event_handler : cancellable_event_handler<fixpoint_reached_event, sim_termination_condition_reached_event, sim_started_event> {
    decider_fixpoint_reached_event_handler();
    void execute(const fixpoint_reached_event&) override;
private:
    i_decider& decider;
};

#endif
