#ifndef CONFLICTED_DETECTOR_HPP
#define CONFLICTED_DETECTOR_HPP

#include "../interfaces/i_conflicted_detector.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/conflicted_event.hpp"

struct conflicted_detector : i_conflicted_detector {
    conflicted_detector();
    void candidates_empty() override;
    void avoidance_empty() override;
private:
    i_event_producer<conflicted_event>& conflicted_producer;
};

#endif
