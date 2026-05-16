#include "../../hpp/infrastructure/goal_activator.hpp"

goal_activator::goal_activator(
    const i_database& db,
    i_lineage_pool& lp,
    i_frontier& frontier,
    i_candidate_activator& candidate_activator,
    i_candidate_deactivator& candidate_deactivator,
    i_goal_initializer& goal_initializer,
    i_candidate_initializer& candidate_initializer,
    i_goal_factory& goal_factory
) : db(db),
    lp(lp),
    frontier(frontier),
    candidate_activator(candidate_activator),
    candidate_deactivator(candidate_deactivator),
    goal_initializer(goal_initializer),
    candidate_initializer(candidate_initializer),
    goal_factory(goal_factory) {}

bool goal_activator::activate(const goal_lineage* gl) {
    // make the goal
    auto g = goal_factory.make();

    // get the raw
    auto raw_g = g.get();

    // insert the goal into the frontier
    frontier.insert(gl, std::move(g));

    // initialize the goal
    goal_initializer.initialize(gl);

    // set up the candidate initializer
    candidate_initializer.seed_expansion(gl);

    // activate candidates
    for (int i = 0; i < db.size(); ++i) {
        // get candidate lineage
        const resolution_lineage* crl = lp.resolution(gl, i);
        // activate candidate
        if (!candidate_activator.activate(crl))
            candidate_deactivator.deactivate(crl);
    }
    
    return raw_g->candidates.size() == 0;
}
