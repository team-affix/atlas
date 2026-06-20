#ifndef UNIT_GOALS_HPP
#define UNIT_GOALS_HPP

#include <optional>
#include <vector>
#include "value_objects/lineage.hpp"

struct unit_goals {
    void push(const goal_lineage*);
    std::optional<const goal_lineage*> pop();
    void clear();
private:
    std::vector<const goal_lineage*> queue_;
};

inline void unit_goals::push(const goal_lineage* gl) {
    queue_.push_back(gl);
}

inline std::optional<const goal_lineage*> unit_goals::pop() {
    if (queue_.empty()) return std::nullopt;
    const goal_lineage* gl = queue_.back();
    queue_.pop_back();
    return gl;
}

inline void unit_goals::clear() {
    queue_.clear();
}

#endif
