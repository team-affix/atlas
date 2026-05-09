#ifndef SOLVED_DETECTOR_SIM_STARTED_EVENT_HANDLER_HPP
#define SOLVED_DETECTOR_SIM_STARTED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_solved_detector.hpp"

struct solved_detector_sim_started_event_handler : event_handler<sim_started_event> {
    solved_detector_sim_started_event_handler();
    void handle(const sim_started_event&) override;
private:
    i_solved_detector& solved_detector;
};

#endif
