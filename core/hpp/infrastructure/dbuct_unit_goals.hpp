#ifndef DBUCT_UNIT_GOALS_HPP
#define DBUCT_UNIT_GOALS_HPP

#include <memory>
#include <optional>
#include <vector>
#include "infrastructure/backtrackable_vector_pop_back.hpp"
#include "infrastructure/backtrackable_vector_push_back.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"

// Delayed-backtracking variant of unit_goals. The LIFO queue is trail-journalled
// (via the abstract ILogTrailAction) so a choice-frame pop restores the exact
// pending unit-goal stack.
template<typename ILogTrailAction>
struct dbuct_unit_goals {
    using vec_t = std::vector<const goal_lineage*>;

    explicit dbuct_unit_goals(ILogTrailAction& t);

    void push(const goal_lineage* gl);
    std::optional<const goal_lineage*> pop();

private:
    tracked<vec_t, ILogTrailAction> queue_;
};

template<typename ILogTrailAction>
dbuct_unit_goals<ILogTrailAction>::dbuct_unit_goals(ILogTrailAction& t) : queue_(t, vec_t{}) {}

template<typename ILogTrailAction>
void dbuct_unit_goals<ILogTrailAction>::push(const goal_lineage* gl) {
    queue_.mutate(std::make_unique<backtrackable_vector_push_back<vec_t>>(gl));
}

template<typename ILogTrailAction>
std::optional<const goal_lineage*> dbuct_unit_goals<ILogTrailAction>::pop() {
    if (queue_.get().empty()) return std::nullopt;
    const goal_lineage* gl = queue_.get().back();
    queue_.mutate(std::make_unique<backtrackable_vector_pop_back<vec_t>>());
    return gl;
}

#endif
