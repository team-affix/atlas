#ifndef GOAL_CANDIDATES_EMPTY_EVENT_HPP
#define GOAL_CANDIDATES_EMPTY_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_candidates_empty_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const goal_candidates_empty_event&);

#endif
