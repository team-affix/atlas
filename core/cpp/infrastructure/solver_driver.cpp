#include "infrastructure/solver_driver.hpp"

solver_driver::solver_driver(coroutine<sim_termination, void> search)
    : search_(std::move(search)), solved_(false) {}

bool solver_driver::next() {
    if (search_.done()) return false;
    search_.resume();
    if (!search_.has_yield()) return false;
    solved_ = search_.consume_yield() == sim_termination::solved;
    return true;
}

bool solver_driver::solved() const { return solved_; }
