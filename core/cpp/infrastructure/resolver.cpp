#include "../../hpp/infrastructure/resolver.hpp"

resolver::resolver(
    const i_database& db,
    i_lineage_pool& lp,
    i_goal_activator& goal_activator,
    i_goal_deactivator& goal_deactivator,
    i_candidate_activator& candidate_activator,
    i_goal_candidates_acceptor& gca,
    i_goal_candidate_deactivator_visitor& gcdv,
    i_conflict_detector& cd,
    i_unit_goal_detector& ugd,
    i_unit_goals& ug)
    :
    db(db),
    lp(lp),
    goal_activator(goal_activator),
    goal_deactivator(goal_deactivator),
    candidate_activator(candidate_activator),
    gca(gca),
    gcdv(gcdv),
    cd(cd),
    ugd(ugd),
    ug(ug) {
}

bool resolver::resolve(const resolution_lineage* rl) {
    // 1. get the rule from db
    const rule& r = db.at(rl->idx);
    // 2. activate the subgoals
    for (int i = 0; i < r.body.size(); ++i) {
        const goal_lineage* gl = lp.goal(rl, i);
        goal_activator.activate(gl);
        for (int j = 0; j < db.size(); ++j)
            candidate_activator.try_activate(lp.resolution(gl, j));
        // check for conflicts
        if (cd.detect(gl))
            return false;
        // check for unit goals
        if (ugd.detect(gl))
            ug.push(gl);
    }
    // get parent goal lineage
    const goal_lineage* gl = rl->parent;
    // 3. deactivate all candidates of parent goal
    gca.accept(gl, gcdv);
    // 4. deactivate the parent goal
    goal_deactivator.deactivate(gl);
    // 5. return success
    return true;
}
