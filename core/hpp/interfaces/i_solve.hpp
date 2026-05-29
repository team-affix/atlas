#ifndef I_SOLVE_HPP
#define I_SOLVE_HPP

#include "value_objects/sim_termination.hpp"
#include "infrastructure/coroutine.hpp"

struct i_solve {
    virtual ~i_solve() = default;
    virtual coroutine<sim_termination, void> solve() = 0;
};

#endif
