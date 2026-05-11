#include "../../../hpp/application/event_handlers/goal_candidates_changed_candidate_eliminated_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_changed_candidate_eliminated_bridge_event_handler::goal_candidates_changed_candidate_eliminated_bridge_event_handler() :
    producer(locator::resolve<i_event_producer<goal_candidates_changed_event>>()) {}

void goal_candidates_changed_candidate_eliminated_bridge_event_handler::handle(const candidate_eliminated_event& e) {
    producer.produce(goal_candidates_changed_event{e.rl->parent});
}
