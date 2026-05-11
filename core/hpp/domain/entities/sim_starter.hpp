#ifndef SIM_STARTER_HPP
#define SIM_STARTER_HPP

#include "../interfaces/i_sim_starter.hpp"
#include "../interfaces/i_goal_resolver.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/sim_started_event.hpp"
#include "../events/fixpoint_reached_event.hpp"
#include "../../utility/i_trail.hpp"

struct sim_starter : i_sim_starter {
    sim_starter();
    void start() override;
    void complete_start() override;
private:
    i_trail& trail;
    i_goal_resolver& goal_resolver;
    i_event_producer<sim_started_event>& sim_started_producer;
    i_event_producer<fixpoint_reached_event>& fixpoint_reached_producer;
};

#endif
