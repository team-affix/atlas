#include "../../hpp/infrastructure/resolver.hpp"

resolver::resolver(
    i_lineage_pool& lp,
    i_goal_activator& goal_activator,
    i_goal_deactivator& goal_deactivator,
    i_get_goal_db_rules& ggdr,
    i_get_goal_candidate_rules& ggcr,
    i_candidate_activator& ca,
    i_candidate_deactivator& cd,
    i_conflict_detector& conflict_detector,
    i_unit_goal_detector& ugd,
    i_push_unit_goal& push_unit_goal)
    :
    lp(lp),
    goal_activator(goal_activator),
    goal_deactivator(goal_deactivator),
    ggdr(ggdr),
    ggcr(ggcr),
    ca(ca),
    cd(cd),
    conflict_detector(conflict_detector),
    ugd(ugd),
    push_unit_goal(push_unit_goal) {
}

void resolver::activate_candidates(const goal_lineage* gl, i_rule_set& rules) {
    auto it = rules.iterate();
    while (!it.done()) {
        auto rr = it.resume();
        if (!rr.has_value())
            continue;
        ca.activate(lp.resolution(gl, rr.value()));
    }
}

void resolver::deactivate_candidates(const goal_lineage* gl, i_rule_set& rules) {
    auto it = rules.iterate();
    while (!it.done()) {
        auto rr = it.resume();
        if (!rr.has_value())
            continue;
        cd.deactivate(lp.resolution(gl, rr.value()));
    }
}

bool resolver::resolve(const resolution_lineage* rl) {
    rule_id r = rl->idx;
    for (auto e : r->body) {
        const goal_lineage* gl = lp.goal(rl, e);
        goal_activator.activate(gl);
        activate_candidates(gl, ggdr.get(gl));
        if (conflict_detector.detect(gl))
            return false;
        if (ugd.detect(gl))
            push_unit_goal.push(gl);
    }
    const goal_lineage* gl = rl->parent;
    deactivate_candidates(gl, ggcr.get(gl));
    goal_deactivator.deactivate(gl);
    return true;
}
