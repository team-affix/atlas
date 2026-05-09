#ifndef CONFLICTED_DETECTOR_AVOIDANCE_EMPTY_EVENT_HANDLER_HPP
#define CONFLICTED_DETECTOR_AVOIDANCE_EMPTY_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/avoidance_empty_event.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/events/sim_started_event.hpp"
#include "../../domain/interfaces/i_conflicted_detector.hpp"

struct conflicted_detector_avoidance_empty_event_handler : cancellable_event_handler<avoidance_empty_event, conflicted_event, sim_started_event> {
    conflicted_detector_avoidance_empty_event_handler();
    void execute(const avoidance_empty_event&) override;
private:
    i_conflicted_detector& conflicted_detector;
};

#endif
