#ifndef DBUCT_ELIMINATION_BACKLOG_HPP
#define DBUCT_ELIMINATION_BACKLOG_HPP

#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "value_objects/elimination_backlog_action.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct dbuct_elimination_backlog {
    void insert_backlogged_elimination(const resolution_lineage* rl);
    bool is_backlogged_elimination(const resolution_lineage* rl) const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<elimination_backlog_action> actions;
    };

    using eliminated_candidates_type =
        std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>>;

    void log(elimination_backlog_action action);
    void undo_action(const elimination_backlog_action& action);

    eliminated_candidates_type eliminated_candidates_;
    std::stack<frame> frame_stack_;
};

inline void dbuct_elimination_backlog::insert_backlogged_elimination(const resolution_lineage* rl) {
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

inline bool dbuct_elimination_backlog::is_backlogged_elimination(const resolution_lineage* rl) const {
    const auto it = eliminated_candidates_.find(rl->parent);
    if (it == eliminated_candidates_.end())
        return false;
    return it->second.contains(rl->idx);
}

inline void dbuct_elimination_backlog::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_elimination_backlog::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_elimination_backlog::log(elimination_backlog_action action) {
    if (!frame_stack_.empty())
        frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_elimination_backlog::undo_action(const elimination_backlog_action& action) {
    if (const auto* ins = std::get_if<elimination_backlog_goal_insert>(&action))
        eliminated_candidates_.erase(ins->gl);
    else {
        const auto& cand = std::get<elimination_backlog_candidate_insert>(action);
        eliminated_candidates_.at(cand.gl).erase(cand.candidate);
    }
}

#endif
