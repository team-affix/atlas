#include "infrastructure/elimination_backlog.hpp"

void elimination_backlog::insert_backlogged_elimination(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    if (!eliminated_candidates_.contains(gl)) {
        eliminated_candidates_.insert({gl, std::unordered_set<rule_id>{}});
        log(elimination_backlog_goal_insert{gl, {}});
    }
    if (!eliminated_candidates_.at(gl).contains(rl->idx)) {
        eliminated_candidates_.at(gl).insert(rl->idx);
        log(elimination_backlog_candidate_insert{gl, rl->idx});
    }
}

bool elimination_backlog::is_backlogged_elimination(const resolution_lineage* rl) const {
    const auto it = eliminated_candidates_.find(rl->parent);
    if (it == eliminated_candidates_.end())
        return false;
    return it->second.contains(rl->idx);
}

void elimination_backlog::push_frame() {
    frame_stack_.push(frame{});
}

void elimination_backlog::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

size_t elimination_backlog::depth() const {
    return frame_stack_.size();
}

void elimination_backlog::log(elimination_backlog_action action) {
    if (!frame_stack_.empty())
        frame_stack_.top().actions_.push_back(std::move(action));
}

void elimination_backlog::undo_action(const elimination_backlog_action& action) {
    if (const auto* ins = std::get_if<elimination_backlog_goal_insert>(&action))
        eliminated_candidates_.erase(ins->gl);
    else {
        const auto& cand = std::get<elimination_backlog_candidate_insert>(action);
        eliminated_candidates_.at(cand.gl).erase(cand.candidate);
    }
}
