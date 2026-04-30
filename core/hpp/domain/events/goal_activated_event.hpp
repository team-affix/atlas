#ifndef GOAL_INSERTED_EVENT_HPP
#define GOAL_INSERTED_EVENT_HPP

#include "../value_objects/lineage.hpp"

struct goal_inserted_event {
    const goal_lineage* gl;
};

#endif
