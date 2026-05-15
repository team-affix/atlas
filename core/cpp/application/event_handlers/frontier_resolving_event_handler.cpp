#include "../../../hpp/application/event_handlers/frontier_resolving_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

frontier_resolving_event_handler::frontier_resolving_event_handler() :
    frontier_(locator::locate<i_frontier>()) {}

void frontier_resolving_event_handler::handle(const resolving_event& event) {
    frontier_.at(event.rl->parent)->choose(event.rl->idx);
}
