#ifndef AVOIDANCE_EMPTY_EVENT_HANDLER_HPP
#define AVOIDANCE_EMPTY_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/avoidance_empty_event.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/events/refuted_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"
#include "../../domain/interfaces/i_get_sim_active.hpp"

struct avoidance_empty_event_handler : cancellable_event_handler<avoidance_empty_event, conflicted_event, sim_started_event> {
    avoidance_empty_event_handler();
    void execute(const avoidance_empty_event&) override;
private:
    i_get_sim_active& get_sim_active;
    i_event_producer<conflicted_event>& conflicted_producer;
    i_event_producer<refuted_event>& refuted_producer;
};

#endif
