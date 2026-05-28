#include "../../hpp/infrastructure/resolver.hpp"

resolver::resolver(
    i_make_goal_lineage& make_goal_lineage,
    i_make_resolution_lineage& make_resolution_lineage,
    i_goal_activator& goal_activator,
    i_goal_deactivator& goal_deactivator,
    i_get_goal_db_rules& get_goal_db_rules,
    i_get_goal_candidate_rules& get_goal_candidate_rules,
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
    get_goal_db_rules(get_goal_db_rules),
    get_goal_candidate_rules(get_goal_candidate_rules),
    candidate_activator(candidate_activator),
    candidate_deactivator(candidate_deactivator),
    conflict_detector(conflict_detector),
    ugd(ugd),
    push_unit_goal(push_unit_goal) {
}

bool resolver::resolve(const resolution_lineage* rl) {
    rule_id r = rl->idx;
    for (auto e : r->body) {
        const goal_lineage* gl = make_goal_lineage.make(rl, e);
        goal_activator.activate(gl);
        auto& db_rules = get_goal_db_rules.get(gl);
        auto db_it = db_rules.iterate();
        while (!db_it.done()) {
            auto rr = db_it.resume();
            if (!rr.has_value())
                continue;
            candidate_activator.activate(make_resolution_lineage.make(gl, rr.value()));
        }
        if (conflict_detector.detect(gl))
            return false;
        if (ugd.detect(gl))
            push_unit_goal.push(gl);
    }
    const goal_lineage* gl = rl->parent;
    auto& candidate_rules = get_goal_candidate_rules.get(gl);
    auto cand_it = candidate_rules.iterate();
    while (!cand_it.done()) {
        auto rr = cand_it.resume();
        if (!rr.has_value())
            continue;
        candidate_deactivator.deactivate(make_resolution_lineage.make(gl, rr.value()));
    }
    goal_deactivator.deactivate(gl);
    return true;
}
