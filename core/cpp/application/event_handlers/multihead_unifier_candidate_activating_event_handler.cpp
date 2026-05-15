#include "../../../hpp/application/event_handlers/multihead_unifier_candidate_activating_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

multihead_unifier_candidate_activating_event_handler::multihead_unifier_candidate_activating_event_handler() :
    multihead_unifier_(locator::locate<i_multihead_unifier>()) {}

void multihead_unifier_candidate_activating_event_handler::handle(const candidate_activating_event& event) {
    multihead_unifier_.add_head(event.rl);
}
