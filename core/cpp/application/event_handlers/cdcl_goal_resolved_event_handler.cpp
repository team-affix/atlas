#include "../../../hpp/application/event_handlers/cdcl_goal_resolved_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

cdcl_goal_resolved_event_handler::cdcl_goal_resolved_event_handler() :
    cdcl(resolver::resolve<i_cdcl>()) {}

void cdcl_goal_resolved_event_handler::handle(const goal_resolved_event& e) {
    cdcl.constrain(e.rl);
}
