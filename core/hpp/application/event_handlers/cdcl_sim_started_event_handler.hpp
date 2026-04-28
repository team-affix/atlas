#ifndef CDCL_SIM_STARTED_EVENT_HANDLER_HPP
#define CDCL_SIM_STARTED_EVENT_HANDLER_HPP

#include "../../../infrastructure/event_handler.hpp"
#include "../../events/sim_started_event.hpp"
#include "cdcl.hpp"

struct cdcl_sim_started_event_handler : event_handler<sim_started_event> {
    cdcl_sim_started_event_handler();
    void operator()(const sim_started_event& e) override;
#ifndef DEBUG
private:
#endif
    cdcl& c;
};

#endif
