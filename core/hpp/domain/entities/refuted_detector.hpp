#ifndef REFUTED_DETECTOR_HPP
#define REFUTED_DETECTOR_HPP

#include "../interfaces/i_refuted_detector.hpp"
#include "../interfaces/i_decision_memory.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/refuted_event.hpp"

struct refuted_detector : i_refuted_detector {
    refuted_detector();
    void conflicted() override;
private:
    i_decision_memory& decision_memory;
    i_event_producer<refuted_event>& refuted_producer;
};

#endif
