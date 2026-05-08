#ifndef DECISION_STORE_DECIDED_EVENT_HANDLER_HPP
#define DECISION_STORE_DECIDED_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/decided_event.hpp"
#include "../../domain/events/sim_cancelled_event.hpp"
#include "../../domain/events/sim_cancellation_reset_event.hpp"
#include "../../domain/interfaces/i_decision_store.hpp"

struct decision_store_decided_event_handler : cancellable_event_handler<decided_event, sim_cancelled_event, sim_cancellation_reset_event> {
    decision_store_decided_event_handler();
    void execute(const decided_event&) override;
private:
    i_decision_store& decision_store;
};

#endif
