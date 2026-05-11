#include "../../../hpp/application/event_handlers/active_eliminator_candidate_not_applicable_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

active_eliminator_candidate_not_applicable_event_handler::active_eliminator_candidate_not_applicable_event_handler() :
    active_eliminator(locator::resolve<i_active_eliminator>()) {}

void active_eliminator_candidate_not_applicable_event_handler::handle(const candidate_not_applicable_event& e) {
    active_eliminator.eliminate(e.rl);
}
