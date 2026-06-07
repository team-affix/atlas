#include "infrastructure/goal_candidates_activator.hpp"

goal_candidates_activator::goal_candidates_activator(locator& loc)
    :
    get_goal_db_rule_ids(loc.locate<i_get_goal_db_rule_ids>()),
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    candidate_activator(loc.locate<i_candidate_activator>()),
    conflict_detector(loc.locate<i_conflict_detector>()),
    ugd(loc.locate<i_detect_unit_goal>()),
    push_unit_goal(loc.locate<i_push_unit_goal>()) {}

bool goal_candidates_activator::activate_goal_candidates(const goal_lineage* gl) {
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
    return true;
}
