#include "../../../hpp/application/event_handlers/fixpoint_reached_repeater_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

fixpoint_reached_repeater_event_handler::fixpoint_reached_repeater_event_handler() :
    fixpoint_reached_producer(resolver::resolve<i_event_producer<fixpoint_reached_event>>()) {}

void fixpoint_reached_repeater_event_handler::execute(const fixpoint_reached_event&) {
    fixpoint_reached_producer.produce(fixpoint_reached_event{});
}
