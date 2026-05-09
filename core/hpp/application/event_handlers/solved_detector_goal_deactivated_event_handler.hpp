#ifndef SOLVED_DETECTOR_GOAL_DEACTIVATED_EVENT_HANDLER_HPP
#define SOLVED_DETECTOR_GOAL_DEACTIVATED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_deactivated_event.hpp"
#include "../../domain/interfaces/i_solved_detector.hpp"

struct solved_detector_goal_deactivated_event_handler : event_handler<goal_deactivated_event> {
    solved_detector_goal_deactivated_event_handler();
    void handle(const goal_deactivated_event&) override;
private:
    i_solved_detector& solved_detector;
};

#endif
