#ifndef ELIMINATION_BACKLOG_GOAL_ACTIVATED_EVENT_HANDLER_HPP
#define ELIMINATION_BACKLOG_GOAL_ACTIVATED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_activated_event.hpp"
#include "../../domain/interfaces/i_elimination_backlog.hpp"

struct elimination_backlog_goal_activated_event_handler : event_handler<goal_activated_event> {
    elimination_backlog_goal_activated_event_handler();
    void handle(const goal_activated_event&) override;
private:
    i_elimination_backlog& elimination_backlog;
};

#endif
