#ifndef DBUCT_GOAL_CANDIDATE_RULES_HPP
#define DBUCT_GOAL_CANDIDATE_RULES_HPP

#include <list>
#include <stack>
#include <unordered_map>
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/goal_candidate_rules_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_goal_candidate_rules {
    dbuct_goal_candidate_rules(ra_rule_id_set_factory& factory);

    const ra_rule_id_set& get(const goal_lineage* gl) const;
    void insert(const goal_lineage* gl);
    void link_goal_candidate(const goal_lineage* gl, rule_id r);
    void unlink_goal_candidate(const goal_lineage* gl, rule_id r);
    void erase(const goal_lineage* gl);

    void push_frame();
    void pop_frame();
    void squash_frame();

private:
    struct frame {
        std::list<goal_candidate_rules_action> actions;
    };

    using map_t = std::unordered_map<const goal_lineage*, ra_rule_id_set>;

    void log(goal_candidate_rules_action action);
    void undo_action(const goal_candidate_rules_action& action);

    ra_rule_id_set_factory& factory_;
    map_t by_goal_;
    std::stack<frame> frame_stack_;
};

inline dbuct_goal_candidate_rules::dbuct_goal_candidate_rules(ra_rule_id_set_factory& factory)
    : factory_(factory) {}

inline const ra_rule_id_set& dbuct_goal_candidate_rules::get(const goal_lineage* gl) const {
    return by_goal_.at(gl);
}

inline void dbuct_goal_candidate_rules::insert(const goal_lineage* gl) {
    ra_rule_id_set value = factory_.make();
    by_goal_.insert({gl, std::move(value)});
    log(goal_candidate_rules_insert{gl, by_goal_.at(gl)});
}

inline void dbuct_goal_candidate_rules::link_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).insert(r);
    log(goal_candidate_rules_at_ra_insert{gl, r});
}

inline void dbuct_goal_candidate_rules::unlink_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).erase(r);
    log(goal_candidate_rules_at_ra_erase{gl, r});
}

inline void dbuct_goal_candidate_rules::erase(const goal_lineage* gl) {
    ra_rule_id_set captured = std::move(by_goal_.at(gl));
    by_goal_.erase(gl);
    log(goal_candidate_rules_erase{gl, std::move(captured)});
}

inline void dbuct_goal_candidate_rules::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_goal_candidate_rules::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_goal_candidate_rules::squash_frame() {
    auto top = std::move(frame_stack_.top());
    frame_stack_.pop();
    auto& parent = frame_stack_.top().actions;
    parent.splice(parent.end(), std::move(top.actions));
}

inline void dbuct_goal_candidate_rules::log(goal_candidate_rules_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_goal_candidate_rules::undo_action(const goal_candidate_rules_action& action) {
    if (const auto* ins = std::get_if<goal_candidate_rules_insert>(&action))
        by_goal_.erase(ins->gl);
    else if (const auto* er = std::get_if<goal_candidate_rules_erase>(&action))
        by_goal_.insert({er->gl, er->value});
    else if (const auto* at_ins = std::get_if<goal_candidate_rules_at_ra_insert>(&action))
        by_goal_.at(at_ins->gl).erase(at_ins->rule);
    else {
        const auto& at_er = std::get<goal_candidate_rules_at_ra_erase>(action);
        by_goal_.at(at_er.gl).insert(at_er.rule);
    }
}

#endif
