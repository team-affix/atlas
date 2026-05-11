#include "../../../hpp/application/event_handlers/elimination_backlog_goal_activated_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

elimination_backlog_goal_activated_event_handler::elimination_backlog_goal_activated_event_handler() :
    elimination_backlog(locator::resolve<i_elimination_backlog>()) {}

void elimination_backlog_goal_activated_event_handler::handle(const goal_activated_event& e) {
    elimination_backlog.goal_activated(e.gl);
}
