#include "infrastructure/pud_search_driver.hpp"
#include <utility>
#include "debug_assert.hpp"

pud_search_driver::pud_search_driver(coroutine<pud_search_step, void> search)
    : search_(std::move(search))
    , last_step_(std::nullopt) {}

bool pud_search_driver::next() {
    if (search_.done())
        return false;
    search_.resume();
    if (!search_.has_yield())
        return false;
    last_step_ = search_.consume_yield();
    return true;
}

pud_search_step pud_search_driver::last_step() const {
    DEBUG_ASSERT(last_step_.has_value());
    return last_step_.value();
}
