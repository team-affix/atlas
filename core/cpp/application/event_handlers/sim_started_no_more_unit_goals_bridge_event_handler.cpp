#include "../../../hpp/application/event_handlers/sim_started_no_more_unit_goals_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_started_no_more_unit_goals_bridge_event_handler::sim_started_no_more_unit_goals_bridge_event_handler() :
    no_more_unit_goals_producer(resolver::resolve<i_event_producer<no_more_unit_goals_event>>()) {}

void sim_started_no_more_unit_goals_bridge_event_handler::handle(const sim_started_event&) {
    no_more_unit_goals_producer.produce(no_more_unit_goals_event{});
}
