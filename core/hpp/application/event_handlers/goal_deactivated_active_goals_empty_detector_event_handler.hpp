#ifndef GOAL_DEACTIVATED_ACTIVE_GOALS_EMPTY_DETECTOR_EVENT_HANDLER_HPP
#define GOAL_DEACTIVATED_ACTIVE_GOALS_EMPTY_DETECTOR_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_deactivated_event.hpp"
#include "../../domain/interfaces/i_active_goals_empty_detector.hpp"

struct goal_deactivated_active_goals_empty_detector_event_handler : event_handler<goal_deactivated_event> {
    goal_deactivated_active_goals_empty_detector_event_handler();
    void handle(const goal_deactivated_event&) override;
private:
    i_active_goals_empty_detector& active_goals_empty_detector;
};

#endif
