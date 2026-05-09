#ifndef REFUTED_DETECTOR_HPP
#define REFUTED_DETECTOR_HPP

#include "../interfaces/i_refuted_detector.hpp"
#include "../interfaces/i_decision_store.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/refuted_event.hpp"

struct refuted_detector : i_refuted_detector {
    refuted_detector();
    void conflicted() override;
private:
    i_decision_store& decision_store;
    i_event_producer<refuted_event>& refuted_producer;
};

#endif
