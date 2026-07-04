#ifndef DBUCT_UNIT_GOALS_HPP
#define DBUCT_UNIT_GOALS_HPP

#include <optional>
#include <vector>
#include "value_objects/lineage.hpp"

// Delayed-backtracking variant of unit_goals.
struct dbuct_unit_goals {
    using snapshot_t = std::vector<const goal_lineage*>;

    void push(const goal_lineage* gl);
    std::optional<const goal_lineage*> pop();

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    std::vector<const goal_lineage*> queue_;
};

inline void dbuct_unit_goals::push(const goal_lineage* gl) { queue_.push_back(gl); }

inline std::optional<const goal_lineage*> dbuct_unit_goals::pop() {
    if (queue_.empty()) return std::nullopt;
    const goal_lineage* gl = queue_.back();
    queue_.pop_back();
    return gl;
}

inline dbuct_unit_goals::snapshot_t dbuct_unit_goals::snapshot() const { return queue_; }
inline void dbuct_unit_goals::restore(snapshot_t s) { queue_ = std::move(s); }

#endif
