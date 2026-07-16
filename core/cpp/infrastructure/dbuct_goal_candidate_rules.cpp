#include "infrastructure/dbuct_goal_candidate_rules.hpp"

dbuct_goal_candidate_rules::dbuct_goal_candidate_rules(ra_rule_id_set_factory& factory)
    : factory_(factory), frame_stack_(std::deque<frame>{frame{}}) {}

const ra_rule_id_set& dbuct_goal_candidate_rules::get(const goal_lineage* gl) const {
    return by_goal_.at(gl);
}

void dbuct_goal_candidate_rules::insert(const goal_lineage* gl) {
    ra_rule_id_set value = factory_.make();
    by_goal_.insert({gl, std::move(value)});
    log(goal_candidate_rules_insert{gl, by_goal_.at(gl)});
}

void dbuct_goal_candidate_rules::link_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).insert(r);
    log(goal_candidate_rules_at_ra_insert{gl, r});
}

void dbuct_goal_candidate_rules::unlink_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).erase(r);
    log(goal_candidate_rules_at_ra_erase{gl, r});
}

void dbuct_goal_candidate_rules::erase(const goal_lineage* gl) {
    ra_rule_id_set captured = std::move(by_goal_.at(gl));
    by_goal_.erase(gl);
    log(goal_candidate_rules_erase{gl, std::move(captured)});
}

void dbuct_goal_candidate_rules::push_frame() { frame_stack_.push(frame{}); }

void dbuct_goal_candidate_rules::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_goal_candidate_rules::log(goal_candidate_rules_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_goal_candidate_rules::undo_action(const goal_candidate_rules_action& action) {
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
