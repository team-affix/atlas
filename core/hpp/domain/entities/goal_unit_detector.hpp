#ifndef GOAL_UNIT_DETECTOR_HPP
#define GOAL_UNIT_DETECTOR_HPP

#include "../interfaces/i_goal_unit_detector.hpp"
#include "../value_objects/lineage.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../interfaces/i_candidates_frontier.hpp"
#include "../events/goal_unit_event.hpp"

struct goal_unit_detector : i_goal_unit_detector {
    goal_unit_detector();
    void candidates_changed(const goal_lineage*) override;
private:
    i_candidates_frontier& candidates_frontier;
    i_event_producer<goal_unit_event>& goal_unit_event_producer;
};

#endif
