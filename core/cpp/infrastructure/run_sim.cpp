#include "infrastructure/run_sim.hpp"

run_sim::run_sim(locator& loc, size_t max_resolutions)
    : max_resolutions_(max_resolutions),
      activate_initial_goals_and_candidates_(loc.locate<i_activate_initial_goals_and_candidates>()),
      solution_detector_(loc.locate<i_solution_detector>()),
      conflict_detector_(loc.locate<i_conflict_detector>()),
      unit_goal_detector_(loc.locate<i_detect_unit_goal>()),
      push_unit_goal_(loc.locate<i_push_unit_goal>()),
      pop_unit_goal_(loc.locate<i_pop_unit_goal>()),
      generate_decision_(loc.locate<i_generate_decision>()),
      elimination_generator_(loc.locate<i_elimination_generator>()),
      elimination_router_(loc.locate<i_elimination_router>()),
      resolver_(loc.locate<i_resolver>()),
      get_unit_resolution_(loc.locate<i_get_unit_resolution>()),
      record_decision_(loc.locate<i_record_decision>()),
      record_resolution_(loc.locate<i_record_resolution>()) {}

sim_termination run_sim::run() {
    if (!activate_initial_goals_and_candidates_.activate_initial_goals_and_candidates())
        return sim_termination::conflicted;

    for (size_t i = 0; i < max_resolutions_; ++i) {
        if (solution_detector_.detect())
            return sim_termination::solved;
        const resolution_lineage* rl = next_resolution();
        auto eliminations = elimination_generator_.constrain(rl);
        while (!eliminations.done()) {
            eliminations.resume();
            if (!eliminations.has_yield())
                continue;
            const resolution_lineage* elim_rl = eliminations.consume_yield();
            if (elimination_router_.route(elim_rl) != elimination_result::eliminated)
                continue;
            const goal_lineage* gl = elim_rl->parent;
            if (conflict_detector_.detect(gl))
                return sim_termination::conflicted;
            if (unit_goal_detector_.detect(gl))
                push_unit_goal_.push(gl);
        }
        if (!resolver_.resolve(rl))
            return sim_termination::conflicted;
    }
    return sim_termination::depth_exceeded;
}

const resolution_lineage* run_sim::next_resolution() {
    const resolution_lineage* rl;
    auto maybe_gl = pop_unit_goal_.pop();
    if (!maybe_gl.has_value()) {
        rl = generate_decision_.generate();
        record_decision_.record_decision(rl);
    } else {
        rl = get_unit_resolution_.get(maybe_gl.value());
    }
    record_resolution_.record_resolution(rl);
    return rl;
}
