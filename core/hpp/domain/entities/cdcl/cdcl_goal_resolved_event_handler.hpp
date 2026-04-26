#ifndef CDCL_GOAL_RESOLVED_EVENT_HANDLER_HPP
#define CDCL_GOAL_RESOLVED_EVENT_HANDLER_HPP

#include "../../../infrastructure/event_handler.hpp"
#include "../../events/goal_resolved_event.hpp"
#include "cdcl.hpp"

struct cdcl_goal_resolved_event_handler : event_handler<goal_resolved_event> {
    cdcl_goal_resolved_event_handler();
    void operator()(const goal_resolved_event& e) override;
#ifndef DEBUG
private:
#endif
    cdcl& c;
};

#endif
