#ifndef I_SOLVER_HPP
#define I_SOLVER_HPP

struct i_solver {
    virtual ~i_solver() = default;
    virtual bool sim_one(bool&) = 0;
};

#endif
