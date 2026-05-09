#ifndef REFUTED_DETECTOR_CONFLICTED_EVENT_HANDLER_HPP
#define REFUTED_DETECTOR_CONFLICTED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/interfaces/i_refuted_detector.hpp"

struct refuted_detector_conflicted_event_handler : event_handler<conflicted_event> {
    refuted_detector_conflicted_event_handler();
    void handle(const conflicted_event&) override;
private:
    i_refuted_detector& detector;
};

#endif
