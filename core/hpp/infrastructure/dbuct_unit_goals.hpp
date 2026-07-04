#ifndef DBUCT_UNIT_GOALS_HPP
#define DBUCT_UNIT_GOALS_HPP

#include <optional>
#include <vector>
#include "value_objects/lineage.hpp"

// Delayed-backtracking variant of unit_goals.
struct dbuct_unit_goals {
    using snapshot_t = std::vector<const goal_lineage*>;

    void push(const goal_lineage* gl) { queue_.push_back(gl); }

    std::optional<const goal_lineage*> pop() {
        if (queue_.empty()) return std::nullopt;
        const goal_lineage* gl = queue_.back();
        queue_.pop_back();
        return gl;
    }

    void clear() { queue_.clear(); }

    snapshot_t snapshot() const { return queue_; }
    void restore(snapshot_t s) { queue_ = std::move(s); }

private:
    std::vector<const goal_lineage*> queue_;
};

#endif
