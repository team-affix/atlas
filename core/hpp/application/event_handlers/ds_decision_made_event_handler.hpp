#ifndef DS_DECISION_MADE_EVENT_HANDLER_HPP
#define DS_DECISION_MADE_EVENT_HANDLER_HPP

#include "../../../infrastructure/event_handler.hpp"
#include "../../events/decision_made_event.hpp"
#include "decision_store.hpp"

struct ds_decision_made_event_handler : event_handler<decision_made_event> {
    ds_decision_made_event_handler();
    void operator()(const decision_made_event&) override;
#ifndef DEBUG
private:
#endif
    decision_store& ds;
};

#endif
