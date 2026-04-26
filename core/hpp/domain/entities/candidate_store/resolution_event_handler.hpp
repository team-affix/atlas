#ifndef RESOLUTION_EVENT_HANDLER_HPP
#define RESOLUTION_EVENT_HANDLER_HPP

#include "../../../infrastructure/event_handler.hpp"
#include "../../events/resolution_event.hpp"
#include "candidate_store.hpp"

struct resolution_event_handler : event_handler<resolution_event> {
    resolution_event_handler();
    void operator()(const resolution_event& e) override;
#ifndef DEBUG
private:
#endif
    candidate_store& cs;
};

#endif
