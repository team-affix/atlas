#include "../../../hpp/application/event_handlers/decider_fixpoint_reached_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

decider_fixpoint_reached_event_handler::decider_fixpoint_reached_event_handler() :
    decider(locator::locate<i_decider>()) {}

void decider_fixpoint_reached_event_handler::execute(const fixpoint_reached_event&) {
    decider.decide();
}
