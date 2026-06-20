#include "infrastructure/ra_active_goals.hpp"

void ra_active_goals::insert_active_goal(const goal_lineage* gl) {
    auto [_, inserted] = index_.emplace(gl, items_.size());
    DEBUG_ASSERT(inserted);
    items_.push_back(gl);
}

void ra_active_goals::erase_active_goal(const goal_lineage* gl) {
    auto it = index_.find(gl);
    DEBUG_ASSERT(it != index_.end());
    size_t idx = it->second;
    const goal_lineage* back = items_.back();
    items_.at(idx) = back;
    index_.at(back) = idx;
    items_.pop_back();
    index_.erase(it);
}

bool ra_active_goals::is_active_goal(const goal_lineage* gl) const {
    return index_.contains(gl);
}

size_t ra_active_goals::active_goals_size() const {
    return items_.size();
}

bool ra_active_goals::empty() const {
    return items_.empty();
}

void ra_active_goals::clear_active_goals() {
    index_.clear();
    items_.clear();
}

const goal_lineage* ra_active_goals::select(size_t index) const {
    return items_.at(index);
}
