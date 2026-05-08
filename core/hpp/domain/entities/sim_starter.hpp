#ifndef SIM_STARTER_HPP
#define SIM_STARTER_HPP

#include "../interfaces/i_sim_starter.hpp"
#include "../interfaces/i_initial_goal_activator.hpp"
#include "../../utility/i_trail.hpp"

struct sim_starter : i_sim_starter {
    sim_starter();
    void start() override;
private:
    i_trail& trail;
    i_initial_goal_activator& initial_goal_activator;
};

#endif
