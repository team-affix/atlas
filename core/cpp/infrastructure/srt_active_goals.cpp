#include "infrastructure/srt_active_goals.hpp"
#include "debug_assert.hpp"

void srt_active_goals::insert_active_goal(const goal_lineage* gl) {
    auto [_, inserted] = in_flight_.emplace(gl);
    DEBUG_ASSERT(inserted);
    DEBUG_ASSERT(tree_.insert(gl));
}

void srt_active_goals::link_srt_goal_batch_parent(const goal_lineage* parent) {
    tree_.link(parent, in_flight_);
}

void srt_active_goals::flush_srt_goal_batch() {
    in_flight_.clear();
}

void srt_active_goals::erase_active_goal(const goal_lineage* gl) {
    DEBUG_ASSERT(tree_.unlink(gl));
    DEBUG_ASSERT(tree_.erase(gl));
}

bool srt_active_goals::is_active_goal(const goal_lineage* gl) const {
    return tree_.leaves().contains(gl);
}

size_t srt_active_goals::active_goals_size() const {
    return tree_.leaves().size();
}

bool srt_active_goals::empty() const {
    return tree_.leaves().empty();
}

void srt_active_goals::clear_active_goals() {
    tree_.clear();
    in_flight_.clear();
}

coroutine<const goal_lineage*, void> srt_active_goals::iterate_root_goals() const {
    for (const goal_lineage* gl : tree_.roots())
        co_yield gl;
}

coroutine<const goal_lineage*, void> srt_active_goals::iterate_child_goals(const goal_lineage* gl) const {
    for (const goal_lineage* child : tree_.children(gl))
        co_yield child;
}
