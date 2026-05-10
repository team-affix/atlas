#include "../../../hpp/application/event_handlers/resolution_memory_goal_resolved_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

resolution_memory_goal_resolved_event_handler::resolution_memory_goal_resolved_event_handler() :
    resolution_memory(resolver::resolve<i_resolution_memory>()) {}

void resolution_memory_goal_resolved_event_handler::handle(const goal_resolved_event& e) {
    resolution_memory.insert(e.rl);
}
