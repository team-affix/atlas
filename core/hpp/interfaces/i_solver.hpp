#ifndef I_SOLVER_HPP
#define I_SOLVER_HPP

#include "../value_objects/solver_yield.hpp"
#include "../utility/state_machine.hpp"

struct i_solver {
    virtual ~i_solver() = default;
    virtual state_machine<solver_yield> solve() = 0;
};

#endif
