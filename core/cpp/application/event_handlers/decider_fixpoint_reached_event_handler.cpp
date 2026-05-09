#include "../../../hpp/application/event_handlers/decider_fixpoint_reached_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

decider_fixpoint_reached_event_handler::decider_fixpoint_reached_event_handler() :
    decider(resolver::resolve<i_decider>()) {}

void decider_fixpoint_reached_event_handler::execute(const fixpoint_reached_event&) {
    decider.decide();
}
