#ifndef PUD_SEARCH_DRIVER_HPP
#define PUD_SEARCH_DRIVER_HPP

#include <optional>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_search_step.hpp"

// Advances a PUD search one step per next(), the PUD counterpart of solver_driver.
struct pud_search_driver {
    pud_search_driver(coroutine<pud_search_step, void> search);
    bool next();
    pud_search_step last_step() const;
private:
    coroutine<pud_search_step, void> search_;
    std::optional<pud_search_step> last_step_;
};

#endif
