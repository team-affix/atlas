#ifndef I_SIM_HPP
#define I_SIM_HPP

#include "../value_objects/sim_termination.hpp"

struct i_sim {
    virtual ~i_sim() = default;
    virtual sim_termination run() = 0;
};

#endif
