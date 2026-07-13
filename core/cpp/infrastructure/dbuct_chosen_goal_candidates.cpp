#include "infrastructure/dbuct_chosen_goal_candidates.hpp"

dbuct_chosen_goal_candidates::dbuct_chosen_goal_candidates() : frame_stack_(std::deque<frame>{frame{}}) {}

std::optional<rule_id> dbuct_chosen_goal_candidates::try_get(const goal_lineage* gl) const {
    const auto it = by_goal_.find(gl);
    if (it == by_goal_.end())
        return std::nullopt;
    return it->second;
}

void dbuct_chosen_goal_candidates::set(const goal_lineage* gl, rule_id r) {
    if (by_goal_.contains(gl)) {
        std::swap(by_goal_.at(gl), r);
        log(chosen_candidate_assign{gl, r});
    } else {
        by_goal_.insert({gl, r});
        log(chosen_candidate_insert{gl, r});
    }
}

void dbuct_chosen_goal_candidates::push_frame() { frame_stack_.push(frame{}); }

void dbuct_chosen_goal_candidates::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_chosen_goal_candidates::log(chosen_candidate_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_chosen_goal_candidates::undo_action(const chosen_candidate_action& action) {
    if (const auto* ins = std::get_if<chosen_candidate_insert>(&action))
        by_goal_.erase(ins->gl);
    else {
        const auto& asg = std::get<chosen_candidate_assign>(action);
        by_goal_.at(asg.gl) = asg.value;
    }
}
