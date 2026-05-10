#ifndef CANDIDATES_EMPTY_DETECTOR_HPP
#define CANDIDATES_EMPTY_DETECTOR_HPP

#include "../interfaces/i_goal_candidates_empty_detector.hpp"
#include "../value_objects/lineage.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/goal_candidates_empty_event.hpp"

struct goal_candidates_empty_detector : i_goal_candidates_empty_detector {
    goal_candidates_empty_detector();
    void candidates_changed(const goal_lineage*) override;
private:
    i_candidates_frontier& candidates_frontier;
    i_event_producer<goal_candidates_empty_event>& goal_candidates_empty_event_producer;
};

#endif
