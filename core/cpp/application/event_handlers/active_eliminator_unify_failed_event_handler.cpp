#include "../../../hpp/application/event_handlers/active_eliminator_unify_failed_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

active_eliminator_unify_failed_event_handler::active_eliminator_unify_failed_event_handler() :
    active_eliminator(locator::locate<i_active_eliminator>()) {}

void active_eliminator_unify_failed_event_handler::handle(const unify_failed_event& e) {
    if (e.rl) active_eliminator.eliminate(e.rl);
}
