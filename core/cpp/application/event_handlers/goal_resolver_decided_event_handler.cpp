#include "../../../hpp/application/event_handlers/goal_resolver_decided_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_resolver_decided_event_handler::goal_resolver_decided_event_handler() :
    goal_resolver(resolver::resolve<i_goal_resolver>()) {
}

void goal_resolver_decided_event_handler::handle(const decided_event& e) {
    goal_resolver.resolve(e.rl);
}
