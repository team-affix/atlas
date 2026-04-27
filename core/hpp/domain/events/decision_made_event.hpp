#ifndef DECISION_MADE_EVENT_HPP
#define DECISION_MADE_EVENT_HPP

#include "../value_objects/lineage.hpp"

struct decision_made_event {
    const resolution_lineage* rl;
};

#endif
