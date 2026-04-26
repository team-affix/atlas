#ifndef ICD_GOAL_INSERTED_EVENT_HANDLER_HPP
#define ICD_GOAL_INSERTED_EVENT_HANDLER_HPP

#include "../../../infrastructure/event_handler.hpp"
#include "../../events/goal_inserted_event.hpp"
#include "initial_condition_checker.hpp"

struct icd_goal_inserted_event_handler : event_handler<goal_inserted_event> {
    icd_goal_inserted_event_handler();
    void operator()(const goal_inserted_event& e) override;
#ifndef DEBUG
private:
#endif
    initial_condition_checker& icd;
};

#endif