#include "infrastructure/srt_active_goals.hpp"

coroutine<const goal_lineage*, void> srt_active_goals::iterate_root_goals() const {
    for (const goal_lineage* gl : tree_.roots())
    co_yield gl;
}

coroutine<const goal_lineage*, void> srt_active_goals::iterate_child_goals(const goal_lineage* gl) const {
    for (const goal_lineage* child : tree_.children(gl))
        co_yield child;
}
