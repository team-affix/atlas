#ifndef DS_SIM_ENDED_EVENT_HANDLER_HPP
#define DS_SIM_ENDED_EVENT_HANDLER_HPP

#include "../../events/sim_ended_event.hpp"
#include "../decision_store/decision_store.hpp"
#include "../../../infrastructure/event_handler.hpp"

struct ds_sim_ended_event_handler : event_handler<sim_ended_event> {
    ds_sim_ended_event_handler();
    void operator()(const sim_ended_event&) override;
#ifndef DEBUG
private:
#endif
    decision_store& ds;
};

#endif
