#include "../../../hpp/application/event_handlers/no_more_unit_goals_repeater_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

no_more_unit_goals_repeater_event_handler::no_more_unit_goals_repeater_event_handler() :
    no_more_unit_goals_producer(resolver::resolve<i_event_producer<no_more_unit_goals_event>>()) {}

void no_more_unit_goals_repeater_event_handler::execute(const no_more_unit_goals_event&) {
    no_more_unit_goals_producer.produce(no_more_unit_goals_event{});
}
