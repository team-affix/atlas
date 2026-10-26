#ifndef PUD_STRUCTURE_WATCH_CHANGE_HPP
#define PUD_STRUCTURE_WATCH_CHANGE_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_structure_watch.hpp"

struct pud_structure_watch_change {
    pud_structure_watch watch;
    std::vector<pud_body_goal_id> changed_goals;
    bool operator==(const pud_structure_watch_change&) const = default;
};

#endif
