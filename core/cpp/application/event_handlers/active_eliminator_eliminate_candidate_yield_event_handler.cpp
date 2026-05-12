#include "../../../hpp/application/event_handlers/active_eliminator_eliminate_candidate_yield_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

active_eliminator_eliminate_candidate_yield_event_handler::active_eliminator_eliminate_candidate_yield_event_handler() :
    active_eliminator(locator::locate<i_active_eliminator>()) {}

void active_eliminator_eliminate_candidate_yield_event_handler::handle(const eliminate_candidate_yield_event&) {
    active_eliminator.resume();
}
