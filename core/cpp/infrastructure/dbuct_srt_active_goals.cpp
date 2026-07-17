#include "infrastructure/dbuct_srt_active_goals.hpp"

dbuct_srt_active_goals::dbuct_srt_active_goals()
    : in_flight_frames_(std::deque<in_flight_frame>{in_flight_frame{}}) {}


void dbuct_srt_active_goals::insert_active_goal(const goal_lineage* gl) {
    in_flight_.insert(gl);
    log_in_flight(srt_in_flight_insert{gl});
    const bool tree_inserted = tree_.insert(gl);
    DEBUG_ASSERT(tree_inserted);
}

void dbuct_srt_active_goals::link_srt_goal_batch_parent(const goal_lineage* parent) {
    tree_.link(parent, in_flight_);
}

void dbuct_srt_active_goals::flush_srt_goal_batch() {
    srt_in_flight_clear captured{std::move(in_flight_)};
    in_flight_.clear();
    log_in_flight(std::move(captured));
}

const goal_lineage* dbuct_srt_active_goals::get_parent_goal(const goal_lineage* gl) const {
    if (tree_.roots().contains(gl))
        return nullptr;
    return tree_.parent(gl);
}

bool dbuct_srt_active_goals::is_active_goal(const goal_lineage* gl) const {
    return tree_.leaves().contains(gl);
}

size_t dbuct_srt_active_goals::active_goals_size() const {
    return tree_.leaves().size();
}

bool dbuct_srt_active_goals::empty() const {
    return tree_.leaves().empty();
}

coroutine<const goal_lineage*, void> dbuct_srt_active_goals::iterate_root_goals() const {
    for (const goal_lineage* gl : tree_.roots())
        co_yield gl;
}

coroutine<const goal_lineage*, void> dbuct_srt_active_goals::iterate_child_goals(
    const goal_lineage* gl) const {
    for (const goal_lineage* child : tree_.children(gl))
        co_yield child;
}

void dbuct_srt_active_goals::push_frame() {
    tree_.push_frame();
    in_flight_frames_.push(in_flight_frame{});
}

void dbuct_srt_active_goals::pop_frame() {
    tree_.pop_frame();
    auto current = std::move(in_flight_frames_.top());
    in_flight_frames_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_in_flight_action(*it);
}

void dbuct_srt_active_goals::log_in_flight(srt_active_goals_action action) {
    DEBUG_ASSERT(!in_flight_frames_.empty());
    in_flight_frames_.top().actions_.push_back(std::move(action));
}

void dbuct_srt_active_goals::undo_in_flight_action(const srt_active_goals_action& action) {
    if (const auto* ins = std::get_if<srt_in_flight_insert>(&action))
        in_flight_.erase(ins->gl);
    else {
        const auto& clr = std::get<srt_in_flight_clear>(action);
        in_flight_ = clr.saved;
    }
}