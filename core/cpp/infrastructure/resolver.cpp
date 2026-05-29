#include "infrastructure/resolver.hpp"

resolver::resolver(locator& loc)
    :
    make_goal_lineage(loc.locate<i_make_goal_lineage>()),
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    goal_activator(loc.locate<i_goal_activator>()),
    goal_deactivator(loc.locate<i_goal_deactivator>()),
    get_rule(loc.locate<i_get_rule>()),
    get_goal_db_rule_ids(loc.locate<i_get_goal_db_rule_ids>()),
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    candidate_activator(loc.locate<i_candidate_activator>()),
    candidate_deactivator(loc.locate<i_candidate_deactivator>()),
    conflict_detector(loc.locate<i_conflict_detector>()),
    ugd(loc.locate<i_detect_unit_goal>()),
    push_unit_goal(loc.locate<i_push_unit_goal>()) {
}

bool resolver::resolve(const resolution_lineage* rl) {
    const rule* rule = get_rule.get(rl->idx);
    for (size_t body_idx = 0; body_idx < rule->body.size(); ++body_idx) {
        const goal_lineage* gl = make_goal_lineage.make_goal_lineage(rl, body_idx);
        goal_activator.activate(gl);
        auto& db_rules = get_goal_db_rule_ids.get(gl);
        auto db_it = db_rules.iterate();
        while (!db_it.done()) {
            db_it.resume();
            if (!db_it.has_yield())
                continue;
            candidate_activator.activate(
                make_resolution_lineage.make_resolution_lineage(gl, db_it.consume_yield()));
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
        cand_it.resume();
        if (!cand_it.has_yield())
            continue;
        candidate_deactivator.deactivate(
            make_resolution_lineage.make_resolution_lineage(gl, cand_it.consume_yield()));
    }
    goal_deactivator.deactivate(gl);
    return true;
}
