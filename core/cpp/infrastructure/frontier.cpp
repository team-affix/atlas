#include "../../hpp/infrastructure/frontier.hpp"

void frontier::insert(const goal_lineage* gl, std::unique_ptr<goal> g) {
    goals_.emplace(gl, std::move(g));
}

bool frontier::contains(const goal_lineage* gl) const {
    return goals_.count(gl) > 0;
}

std::unique_ptr<goal>& frontier::at(const goal_lineage* gl) {
    return goals_.at(gl);
}

const std::unique_ptr<goal>& frontier::at(const goal_lineage* gl) const {
    return goals_.at(gl);
}

void frontier::erase(const goal_lineage* gl) {
    goals_.erase(gl);
}

void frontier::clear() {
    goals_.clear();
}

size_t frontier::size() const {
    return goals_.size();
}

void frontier::accept(i_visitor<const std::pair<const goal_lineage* const, std::unique_ptr<goal>>&>& v) const {
    for (const auto& entry : goals_)
        v.visit(entry);
}
