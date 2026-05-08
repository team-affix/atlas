#ifndef NO_MORE_UNIT_GOALS_REPEATER_EVENT_HANDLER_HPP
#define NO_MORE_UNIT_GOALS_REPEATER_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/no_more_unit_goals_event.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct no_more_unit_goals_repeater_event_handler : cancellable_event_handler<no_more_unit_goals_event, conflicted_event, sim_started_event> {
    no_more_unit_goals_repeater_event_handler();
    void execute(const no_more_unit_goals_event&) override;
private:
    i_event_producer<no_more_unit_goals_event>& no_more_unit_goals_producer;
};

#endif
