#ifndef DBUCT_CHOSEN_GOAL_CANDIDATES_HPP
#define DBUCT_CHOSEN_GOAL_CANDIDATES_HPP

#include <list>
#include <optional>
#include <stack>
#include <unordered_map>
#include "value_objects/chosen_candidate_action.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct dbuct_chosen_goal_candidates {
    std::optional<rule_id> try_get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, rule_id r);

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<chosen_candidate_action> actions;
    };

    using map_t = std::unordered_map<const goal_lineage*, rule_id>;

    void log(chosen_candidate_action action);
    void undo_action(const chosen_candidate_action& action);

    map_t by_goal_;
    std::stack<frame> frame_stack_;
};

inline std::optional<rule_id> dbuct_chosen_goal_candidates::try_get(const goal_lineage* gl) const {
    const auto it = by_goal_.find(gl);
    if (it == by_goal_.end())
        return std::nullopt;
    return it->second;
}

inline void dbuct_chosen_goal_candidates::set(const goal_lineage* gl, rule_id r) {
    if (by_goal_.contains(gl)) {
        std::swap(by_goal_.at(gl), r);
        log(chosen_candidate_assign{gl, r});
    } else {
        by_goal_.insert({gl, r});
        log(chosen_candidate_insert{gl, r});
    }
}

inline void dbuct_chosen_goal_candidates::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_chosen_goal_candidates::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_chosen_goal_candidates::log(chosen_candidate_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_chosen_goal_candidates::undo_action(const chosen_candidate_action& action) {
    if (const auto* ins = std::get_if<chosen_candidate_insert>(&action))
        by_goal_.erase(ins->gl);
    else {
        const auto& asg = std::get<chosen_candidate_assign>(action);
        by_goal_.at(asg.gl) = asg.value;
    }
}

#endif
