#ifndef SIM_STARTER_HPP
#define SIM_STARTER_HPP

#include "../interfaces/i_sim_starter.hpp"
#include "../interfaces/i_initial_goal_activator.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/sim_started_event.hpp"
#include "../events/no_more_unit_goals_event.hpp"
#include "../../utility/i_trail.hpp"

struct sim_starter : i_sim_starter {
    sim_starter();
    void start() override;
    void complete_start() override;
private:
    i_trail& trail;
    i_initial_goal_activator& initial_goal_activator;
    i_event_producer<sim_started_event>& sim_started_producer;
    i_event_producer<no_more_unit_goals_event>& no_more_unit_goals_producer;
};

#endif
