#ifndef SIM_TEARDOWN_HPP
#define SIM_TEARDOWN_HPP

#include "../interfaces/i_sim_teardown.hpp"
#include "../utility/i_trail.hpp"

struct sim_teardown : i_sim_teardown {
    sim_teardown(
        i_trail& trail);
    void teardown() override;
private:
    i_trail& trail;
};

#endif
