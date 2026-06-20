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

inline solver_driver::solver_driver(coroutine<sim_termination, void> search)
    : search_(std::move(search)), solved_(false) {}

inline bool solver_driver::next() {
    if (search_.done()) return false;
    search_.resume();
    if (!search_.has_yield()) return false;
    solved_ = search_.consume_yield() == sim_termination::solved;
    return true;
}

inline bool solver_driver::solved() const { return solved_; }

#endif
