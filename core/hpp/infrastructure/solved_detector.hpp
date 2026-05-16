#ifndef SOLVED_DETECTOR_HPP
#define SOLVED_DETECTOR_HPP

#include "../interfaces/i_solved_detector.hpp"
#include "../interfaces/i_frontier.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/solved_event.hpp"

struct solved_detector : i_solved_detector {
    solved_detector();
    void detect_solved() override;
private:
    i_frontier& f;
    i_event_producer<solved_event>& solved_producer;
};

#endif
