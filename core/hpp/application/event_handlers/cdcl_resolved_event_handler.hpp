#ifndef CDCL_RESOLVED_EVENT_HANDLER_HPP
#define CDCL_RESOLVED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/resolved_event.hpp"
#include "../../domain/interfaces/i_cdcl.hpp"

struct cdcl_resolved_event_handler : event_handler<resolved_event> {
    cdcl_resolved_event_handler();
    void handle(const resolved_event&) override;
private:
    i_cdcl& cdcl;
};

#endif
