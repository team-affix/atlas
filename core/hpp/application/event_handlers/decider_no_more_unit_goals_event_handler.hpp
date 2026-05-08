#ifndef DECIDER_NO_MORE_UNIT_GOALS_EVENT_HANDLER_HPP
#define DECIDER_NO_MORE_UNIT_GOALS_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/no_more_unit_goals_event.hpp"
#include "../../domain/events/sim_cancelled_event.hpp"
#include "../../domain/events/sim_cancellation_reset_event.hpp"
#include "../../domain/interfaces/i_decider.hpp"

struct decider_no_more_unit_goals_event_handler : cancellable_event_handler<no_more_unit_goals_event, sim_cancelled_event, sim_cancellation_reset_event> {
    decider_no_more_unit_goals_event_handler();
    void execute(const no_more_unit_goals_event&) override;
private:
    i_decider& decider;
};

#endif
