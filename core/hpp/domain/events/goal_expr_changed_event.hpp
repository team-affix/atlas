#ifndef GOAL_EXPR_CHANGED_EVENT_HPP
#define GOAL_EXPR_CHANGED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_expr_changed_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const goal_expr_changed_event&);

#endif
