#include "infrastructure/active_goals.hpp"

void active_goals::insert_active_goal(const goal_lineage* gl) {
    goals_.insert(gl);
}

void active_goals::erase_active_goal(const goal_lineage* gl) {
    goals_.erase(gl);
}

bool active_goals::is_active_goal(const goal_lineage* gl) const {
    return goals_.contains(gl);
}

state_machine<const goal_lineage*> active_goals::iterate_active_goals() const {
    for (const goal_lineage* gl : goals_)
        co_yield gl;
}

size_t active_goals::active_goals_size() const {
    return goals_.size();
}

bool active_goals::empty() const {
    return goals_.empty();
}

void active_goals::clear_active_goals() {
    goals_.clear();
}
