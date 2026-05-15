#include "../../../hpp/application/event_handlers/multihead_unifier_resolving_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

multihead_unifier_resolving_event_handler::multihead_unifier_resolving_event_handler() :
    multihead_unifier_(locator::locate<i_multihead_unifier>()) {}

void multihead_unifier_resolving_event_handler::handle(const resolving_event& event) {
    if (!event.rl) { return; }
    multihead_unifier_.accept_head(event.rl);
}
