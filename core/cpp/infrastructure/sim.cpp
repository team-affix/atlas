#include "../../hpp/infrastructure/sim.hpp"

sim::sim(
    size_t max_resolutions,
    i_unit_goals& ug,
    i_decision_generator& dg,
    i_elimination_generator& eg,
    i_elimination_router& er,
    i_resolver& r,
    i_goal_candidates_acceptor& gca,
    i_goal_candidates_extractor_visitor_factory& gcevf)
    :
    max_resolutions(max_resolutions),
    ug(ug),
    dg(dg),
    eg(eg),
    er(er),
    r(r),
    gca(gca),
    gcevf(gcevf) {
}

sim_termination sim::run() {
    for (size_t i = 0; i < max_resolutions; ++i) {
        // 0. check if the sim is solved
        if (sd.detect())
            return sim_termination::solved;
        // 1. get the next resolution
        const resolution_lineage* rl = next_resolution();
        // 2. generate eliminations from this
        auto eliminations = eg.constrain(rl);
        // 3. route the eliminations
        while (!eliminations.done()) {
            auto res = eliminations.resume();
            if (!res.has_value())
                continue;
            auto elim_result = er.route(res.value());
            if (handle_elimination_result(elim_result))
                return sim_termination::conflicted;
        }
        // 4. resolve given rl
        r.resolve(rl);
    }
    return sim_termination::depth_exceeded;
}

const resolution_lineage* sim::next_resolution() {
    // if no unit goals, generate a new decision
    if (ug.empty())
        return dg.generate();

    // 1. get the next unit goal
    const goal_lineage* gl = ug.pop();
    // 2. determine which candidate it still has
    std::unordered_set<const resolution_lineage*> extracted_candidates;
    auto vis = gcevf.make(extracted_candidates);
    gca.accept(gl, *vis);
    // 3. return the first and only candidate
    return *extracted_candidates.begin();
}

bool sim::handle_elimination_result(const elimination_result& result) {
    if (const auto* unit = std::get_if<goal_made_unit>(&result))
        ug.push(unit->gl);
    else if (std::holds_alternative<goal_candidates_empty>(result))
        return true;
    return false;
}
