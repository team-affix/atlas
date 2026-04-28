#ifndef RS_GOAL_RESOLVED_EVENT_HANDLER_HPP
#define RS_GOAL_RESOLVED_EVENT_HANDLER_HPP

#include "../../../infrastructure/event_handler.hpp"
#include "../../events/goal_resolved_event.hpp"
#include "resolution_store.hpp"

struct rs_goal_resolved_event_handler : event_handler<goal_resolved_event> {
    rs_goal_resolved_event_handler();
    void operator()(const goal_resolved_event&) override;
#ifndef DEBUG
private:
#endif
    resolution_store& rs;
};

#endif
