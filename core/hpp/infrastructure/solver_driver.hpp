#ifndef SOLVER_DRIVER_HPP
#define SOLVER_DRIVER_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/sim_termination.hpp"

struct solver_driver {
    explicit solver_driver(coroutine<sim_termination, void> search);
    bool next();
    bool solved() const;
private:
    coroutine<sim_termination, void> search_;
    bool solved_;
};

#endif
