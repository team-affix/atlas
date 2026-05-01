#include "../../hpp/infrastructure/active_goal_store.hpp"

active_goal_store::active_goal_store() {
}

void active_goal_store::insert(const goal_lineage* gl) {
    active_goals.insert(gl);
}

void active_goal_store::erase(const goal_lineage* gl) {
    active_goals.erase(gl);
}

void active_goal_store::clear() {
    active_goals.clear();
}

bool active_goal_store::contains(const goal_lineage* gl) {
    return active_goals.contains(gl);
}
