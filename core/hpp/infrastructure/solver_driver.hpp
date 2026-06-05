#ifndef SOLVER_DRIVER_HPP
#define SOLVER_DRIVER_HPP

#include "infrastructure/coroutine.hpp"
#include "interfaces/i_advance_solver_session.hpp"
#include "interfaces/i_get_solver_session_solved.hpp"
#include "value_objects/sim_termination.hpp"

struct solver_driver
    : i_advance_solver_session
    , i_get_solver_session_solved {
    explicit solver_driver(coroutine<sim_termination, void> search);

    bool next() override;
    bool solved() const override;

private:
    coroutine<sim_termination, void> search_;
    bool solved_;
};

#endif
