#include "../../../hpp/application/event_handlers/conflicted_sim_termination_condition_reached_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

conflicted_sim_termination_condition_reached_bridge_event_handler::conflicted_sim_termination_condition_reached_bridge_event_handler() :
    producer(locator::resolve<i_event_producer<sim_termination_condition_reached_event>>()) {}

void conflicted_sim_termination_condition_reached_bridge_event_handler::handle(const conflicted_event&) {
    producer.produce(sim_termination_condition_reached_event{});
}
