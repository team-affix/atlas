#include "../../hpp/infrastructure/sim.hpp"

#include <unordered_set>

sim::sim(
    size_t max_resolutions,
    i_lineage_pool& lp,
    i_solution_detector& sd,
    i_conflict_detector& cd,
    i_unit_goal_detector& ugd,
    i_push_unit_goal& push_unit_goal,
    i_pop_unit_goal& pop_unit_goal,
    i_check_unit_goals_empty& check_unit_goals_empty,
    i_decision_generator& dg,
    i_elimination_generator& eg,
    i_elimination_router& er,
    i_resolver& r,
    i_get_goal_candidate_rules& ggcr)
    :
    max_resolutions(max_resolutions),
    lp(lp),
    sd(sd),
    cd(cd),
    ugd(ugd),
    push_unit_goal(push_unit_goal),
    pop_unit_goal(pop_unit_goal),
    check_unit_goals_empty(check_unit_goals_empty),
    dg(dg),
    eg(eg),
    er(er),
    r(r),
    ggcr(ggcr) {
}

void sim::set_up() {}

sim_termination sim::run() {
    for (size_t i = 0; i < max_resolutions; ++i) {
        if (sd.detect())
            return sim_termination::solved;
        const resolution_lineage* rl = next_resolution();
        auto eliminations = eg.constrain(rl);
        while (!eliminations.done()) {
            auto res = eliminations.resume();
            if (!res.has_value())
                continue;
            const resolution_lineage* elim_rl = res.value();
            er.route(elim_rl);
            const goal_lineage* gl = elim_rl->parent;
            if (cd.detect(gl))
                return sim_termination::conflicted;
            if (ugd.detect(gl))
                push_unit_goal.push(gl);
        }
        if (!r.resolve(rl))
            return sim_termination::conflicted;
    }
    return sim_termination::depth_exceeded;
}

void sim::tear_down() {}

const resolution_lineage* sim::next_resolution() {
    if (check_unit_goals_empty.empty())
        return dg.generate();

    const goal_lineage* gl = pop_unit_goal.pop();
    auto& candidate_rules = ggcr.get(gl);
    std::unordered_set<const rule*> extracted_candidates;
    auto it = candidate_rules.iterate();
    while (!it.done()) {
        auto rr = it.resume();
        if (!rr.has_value())
            continue;
        extracted_candidates.insert(rr.value());
    }
    return lp.resolution(gl, *extracted_candidates.begin());
}
