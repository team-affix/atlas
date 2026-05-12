#include "../../../hpp/application/event_handlers/unify_synchronizer_candidate_activating_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

unify_synchronizer_candidate_activating_event_handler::unify_synchronizer_candidate_activating_event_handler() :
    unify_synchronizer(locator::locate<i_unify_synchronizer>()) {}

void unify_synchronizer_candidate_activating_event_handler::handle(const candidate_activating_event& e) {
    unify_synchronizer.register_candidate(e.rl);
}
