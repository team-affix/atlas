#ifndef DBUCT_UNIT_GOALS_HPP
#define DBUCT_UNIT_GOALS_HPP

#include <memory>
#include <optional>
#include <vector>
#include "infrastructure/backtrackable_vector_pop_back.hpp"
#include "infrastructure/backtrackable_vector_push_back.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"

// Delayed-backtracking variant of unit_goals. The LIFO queue is trail-journalled
// so a choice-frame pop restores the exact pending unit-goal stack.
struct dbuct_unit_goals {
    using vec_t = std::vector<const goal_lineage*>;

    explicit dbuct_unit_goals(trail& t);

    void push(const goal_lineage* gl);
    std::optional<const goal_lineage*> pop();

private:
    tracked<vec_t, trail> queue_;
};

inline dbuct_unit_goals::dbuct_unit_goals(trail& t) : queue_(t, vec_t{}) {}

inline void dbuct_unit_goals::push(const goal_lineage* gl) {
    queue_.mutate(std::make_unique<backtrackable_vector_push_back<vec_t>>(gl));
}

inline std::optional<const goal_lineage*> dbuct_unit_goals::pop() {
    if (queue_.get().empty()) return std::nullopt;
    const goal_lineage* gl = queue_.get().back();
    queue_.mutate(std::make_unique<backtrackable_vector_pop_back<vec_t>>());
    return gl;
}

#endif
