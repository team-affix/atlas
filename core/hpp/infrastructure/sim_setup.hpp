#ifndef SIM_SETUP_HPP
#define SIM_SETUP_HPP

#include "../interfaces/i_sim_setup.hpp"
#include "../utility/i_trail.hpp"

struct sim_setup : i_sim_setup {
    sim_setup(
        i_trail& trail);
    void setup() override;
private:
    i_trail& trail;
};

#endif
