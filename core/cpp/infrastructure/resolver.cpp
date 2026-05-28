#include "infrastructure/resolver.hpp"

resolver::resolver(
    i_make_goal_lineage& make_goal_lineage,
    i_make_resolution_lineage& make_resolution_lineage,
    i_goal_activator& goal_activator,
    i_goal_deactivator& goal_deactivator,
    i_get_rule& get_rule,
    i_get_goal_db_rule_ids& get_goal_db_rule_ids,
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids,
    i_candidate_activator& candidate_activator,
    i_candidate_deactivator& candidate_deactivator,
    i_conflict_detector& conflict_detector,
    i_detect_unit_goal& ugd,
    i_push_unit_goal& push_unit_goal)
    :
    make_goal_lineage(make_goal_lineage),
    make_resolution_lineage(make_resolution_lineage),
    goal_activator(goal_activator),
    goal_deactivator(goal_deactivator),
    get_rule(get_rule),
    get_goal_db_rule_ids(get_goal_db_rule_ids),
    get_goal_candidate_rule_ids(get_goal_candidate_rule_ids),
    candidate_activator(candidate_activator),
    candidate_deactivator(candidate_deactivator),
    conflict_detector(conflict_detector),
    ugd(ugd),
    push_unit_goal(push_unit_goal) {
}

const resolution_lineage* resolver::get_unit_resolution(const goal_lineage* gl) {
    auto& candidate_rules = get_goal_candidate_rule_ids.get(gl);
    auto it = candidate_rules.iterate();
    while (!it.done()) {
        auto rr = it.resume();
        if (!rr.has_value())
            continue;
        return make_resolution_lineage.make_resolution_lineage(gl, rr.value());
    }
    return nullptr;
}

bool resolver::resolve(const resolution_lineage* rl) {
    const rule* rule = get_rule.get(rl->idx);
    for (size_t body_idx = 0; body_idx < rule->body.size(); ++body_idx) {
        const goal_lineage* gl = make_goal_lineage.make_goal_lineage(rl, body_idx);
        goal_activator.activate(gl);
        auto& db_rules = get_goal_db_rule_ids.get(gl);
        auto db_it = db_rules.iterate();
        while (!db_it.done()) {
            auto rr = db_it.resume();
            if (!rr.has_value())
                continue;
            candidate_activator.activate(make_resolution_lineage.make_resolution_lineage(gl, rr.value()));
        }
        if (conflict_detector.detect(gl))
            return false;
        if (ugd.detect(gl))
            push_unit_goal.push(gl);
    }
    const goal_lineage* gl = rl->parent;
    auto candidate_rules = get_goal_candidate_rule_ids.get(gl).copy();
    auto cand_it = candidate_rules->iterate();
    while (!cand_it.done()) {
        auto rr = cand_it.resume();
        if (!rr.has_value())
            continue;
        candidate_deactivator.deactivate(make_resolution_lineage.make_resolution_lineage(gl, rr.value()));
    }
    goal_deactivator.deactivate(gl);
    return true;
}
