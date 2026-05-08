#ifndef ACTIVE_GOALS_EMPTY_DETECTOR_HPP
#define ACTIVE_GOALS_EMPTY_DETECTOR_HPP

#include "../interfaces/i_active_goals_empty_detector.hpp"
#include "../interfaces/i_active_goal_store.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/active_goals_empty_event.hpp"

struct active_goals_empty_detector : i_active_goals_empty_detector {
    active_goals_empty_detector();
    void goal_deactivated() override;
private:
    i_active_goal_store& active_goal_store;
    i_event_producer<active_goals_empty_event>& active_goals_empty_producer;
};

#endif
