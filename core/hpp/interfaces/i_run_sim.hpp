#ifndef I_RUN_SIM_HPP
#define I_RUN_SIM_HPP

#include "../value_objects/sim_termination.hpp"

struct i_run_sim {
    virtual ~i_run_sim() = default;
    virtual sim_termination run() = 0;
};

#endif
