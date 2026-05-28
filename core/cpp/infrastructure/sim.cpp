#include "infrastructure/sim.hpp"

sim::sim(locator& loc, size_t max_resolutions)
    :
    max_resolutions(max_resolutions),
    push_trail_frame(loc.locate<i_push_trail_frame>()),
    pop_trail_frame(loc.locate<i_pop_trail_frame>()),
    get_initial_goal_count(loc.locate<i_get_initial_goal_count>()),
    activate_initial_goal(loc.locate<i_activate_initial_goal>()),
    make_initial_goal_lineage(loc.locate<i_make_initial_goal_lineage>()),
    get_goal_db_rule_ids(loc.locate<i_get_goal_db_rule_ids>()),
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    candidate_activator(loc.locate<i_candidate_activator>()),
    sd(loc.locate<i_solution_detector>()),
    cd(loc.locate<i_conflict_detector>()),
    ugd(loc.locate<i_detect_unit_goal>()),
    push_unit_goal(loc.locate<i_push_unit_goal>()),
    pop_unit_goal(loc.locate<i_pop_unit_goal>()),
    generate_decision(loc.locate<i_generate_decision>()),
    eg(loc.locate<i_elimination_generator>()),
    er(loc.locate<i_elimination_router>()),
    r(loc.locate<i_resolver>()),
    get_unit_resolution(loc.locate<i_get_unit_resolution>()),
    record_decision(loc.locate<i_record_decision>()),
    record_resolution(loc.locate<i_record_resolution>()),
    clear_unit_goals(loc.locate<i_clear_unit_goals>()),
    clear_recorded_decisions(loc.locate<i_clear_recorded_decisions>()),
    clear_recorded_resolutions(loc.locate<i_clear_recorded_resolutions>()),
    deactivated_candidate_memory(loc.locate<i_deactivated_candidate_memory>()),
    clear_goal_candidate_rule_ids(loc.locate<i_clear_goal_candidate_rule_ids>()),
    clear_goal_exprs(loc.locate<i_clear_goal_exprs>()),
    clear_active_goals(loc.locate<i_clear_active_goals>()),
    clear_candidate_translation_maps(loc.locate<i_clear_candidate_translation_maps>()),
    clear_mhu_heads(loc.locate<i_clear_mhu_heads>()),
    clear_bindings(loc.locate<i_clear_bindings>()),
    trim_unpinned_lineages(loc.locate<i_trim_unpinned_lineages>()) {
}

void sim::set_up() {
    push_trail_frame.push();
}

sim_termination sim::run() {
    // --- activate initial goals and db candidates ---
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
        if (cd.detect(gl))
            return sim_termination::conflicted;
        if (ugd.detect(gl))
            push_unit_goal.push(gl);
    }
    
    // --- resolution loop ---
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

void sim::tear_down() {
    pop_trail_frame.pop();
    clear_unit_goals.clear();
    clear_recorded_decisions.clear_recorded_decisions();
    clear_recorded_resolutions.clear_recorded_resolutions();
    deactivated_candidate_memory.clear();
    clear_goal_candidate_rule_ids.clear_goal_candidate_rule_ids();
    clear_goal_exprs.clear_goal_exprs();
    clear_active_goals.clear_active_goals();
    clear_candidate_translation_maps.clear_candidate_translation_maps();
    clear_mhu_heads.clear_mhu_heads();
    clear_bindings.clear_bindings();
    trim_unpinned_lineages.trim();
}

const resolution_lineage* sim::next_resolution() {
    const resolution_lineage* rl;
    auto maybe_gl = pop_unit_goal.pop();
    if (!maybe_gl.has_value()) {
        rl = generate_decision.generate();
        record_decision.record_decision(rl);
    } else {
        rl = get_unit_resolution.get(maybe_gl.value());
    }
    record_resolution.record_resolution(rl);
    return rl;
}
