#ifndef SIM_HPP
#define SIM_HPP

#include "../interfaces/i_sim.hpp"
#include "../interfaces/i_resolver.hpp"

struct sim : i_sim {
    sim();
    sim_termination run() override;
};

#endif
