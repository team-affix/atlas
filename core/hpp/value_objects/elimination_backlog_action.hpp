#ifndef ELIMINATION_BACKLOG_ACTION_HPP
#define ELIMINATION_BACKLOG_ACTION_HPP

#include <variant>
#include "value_objects/elimination_backlog_goal_insert.hpp"
#include "value_objects/elimination_backlog_candidate_insert.hpp"

using elimination_backlog_action = std::variant<
    elimination_backlog_goal_insert,
    elimination_backlog_candidate_insert>;

#endif
