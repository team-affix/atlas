#include "../../../hpp/application/event_handlers/decider_no_more_unit_goals_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

decider_no_more_unit_goals_event_handler::decider_no_more_unit_goals_event_handler() :
    decider(resolver::resolve<i_decider>()) {}

void decider_no_more_unit_goals_event_handler::execute(const no_more_unit_goals_event&) {
    decider.decide();
}
