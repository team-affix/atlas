#ifndef I_SOLVE_HPP
#define I_SOLVE_HPP

#include "../value_objects/sim_termination.hpp"
#include "../utility/state_machine.hpp"

struct i_solve {
    virtual ~i_solve() = default;
    virtual state_machine<sim_termination> solve() = 0;
};

#endif
