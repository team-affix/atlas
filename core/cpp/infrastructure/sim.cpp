#include "../../hpp/infrastructure/sim.hpp"

#include <unordered_set>

sim::sim(
    size_t max_resolutions,
    i_trail& trail,
    i_get_initial_goal_count& get_initial_goal_count,
    i_activate_initial_goal& activate_initial_goal,
    i_make_initial_goal_lineage& make_initial_goal_lineage,
    i_get_goal_db_rule_ids& get_goal_db_rule_ids,
    i_make_resolution_lineage& make_resolution_lineage,
    i_candidate_activator& candidate_activator,
    i_solution_detector& sd,
    i_conflict_detector& cd,
    i_detect_unit_goal& ugd,
    i_push_unit_goal& push_unit_goal,
    i_pop_unit_goal& pop_unit_goal,
    i_generate_decision& generate_decision,
    i_elimination_generator& eg,
    i_elimination_router& er,
    i_resolver& r,
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids)
    :
    max_resolutions(max_resolutions),
    trail(trail),
    get_initial_goal_count(get_initial_goal_count),
    activate_initial_goal(activate_initial_goal),
    make_initial_goal_lineage(make_initial_goal_lineage),
    get_goal_db_rule_ids(get_goal_db_rule_ids),
    make_resolution_lineage(make_resolution_lineage),
    candidate_activator(candidate_activator),
    sd(sd),
    cd(cd),
    ugd(ugd),
    push_unit_goal(push_unit_goal),
    pop_unit_goal(pop_unit_goal),
    generate_decision(generate_decision),
    eg(eg),
    er(er),
    r(r),
    get_goal_candidate_rule_ids(get_goal_candidate_rule_ids) {
}

void sim::set_up() {
    trail.push();

    for (size_t i = 0; i < get_initial_goal_count.count(); ++i) {
        activate_initial_goal.activate_initial_goal(i);
        const goal_lineage* gl = make_initial_goal_lineage.make(i);
        auto& rules = get_goal_db_rule_ids.get(gl);
        auto it = rules.iterate();
        while (!it.done()) {
            auto rr = it.resume();
            if (!rr.has_value())
                continue;
            candidate_activator.activate(make_resolution_lineage.make_resolution_lineage(gl, rr.value()));
        }
    }
}

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
    auto maybe_gl = pop_unit_goal.pop();
    if (!maybe_gl.has_value())
        return generate_decision.generate();

    const goal_lineage* gl = maybe_gl.value();
    auto& candidate_rules = get_goal_candidate_rule_ids.get(gl);
    std::unordered_set<rule_id> extracted_candidates;
    auto it = candidate_rules.iterate();
    while (!it.done()) {
        auto rr = it.resume();
        if (!rr.has_value())
            continue;
        extracted_candidates.insert(rr.value());
    }
    return make_resolution_lineage.make_resolution_lineage(gl, *extracted_candidates.begin());
}
