#include "../../../hpp/application/event_handlers/unify_synchronizer_resolving_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

unify_synchronizer_resolving_event_handler::unify_synchronizer_resolving_event_handler() :
    unify_synchronizer(locator::locate<i_unify_synchronizer>()) {}

void unify_synchronizer_resolving_event_handler::handle(const resolving_event& e) {
    if (e.rl) unify_synchronizer.accept(e.rl);
}
