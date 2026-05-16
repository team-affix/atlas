#include "../../hpp/infrastructure/elimination_processor.hpp"

elimination_processor::elimination_processor(
    const i_frontier& frontier,
    const i_deactivated_goal_memory& dgm,
    i_elimination_generator& eg,
    i_elimination_backlog& eb,
    i_candidate_deactivator& cd,
    i_conflict_detector& conflict_detector)
    :
    frontier(frontier),
    dgm(dgm),
    eg(eg),
    eb(eb),
    cd(cd),
    conflict_detector(conflict_detector) {}

bool elimination_processor::process(const resolution_lineage* rl) {
    // constrain the resolution
    auto new_eliminations = eg.constrain(rl);

    // route the eliminations properly
    while (!new_eliminations.done()) {
        // get the next elimination
        auto elimination = new_eliminations.resume();
        
        if (!elimination.has_value())
            continue;

        // get the eliminated resolution lineage
        const resolution_lineage* eliminated_rl = elimination.value();

        // get parent goal lineage
        const goal_lineage* parent_gl = eliminated_rl->parent;

        // if the parent goal is deactivated, just skip
        if (dgm.contains(parent_gl))
            continue;

        // if frontier does not contain parent, add to backlog
        if (!frontier.contains(parent_gl)) {
            eb.push(eliminated_rl);
            continue;
        }

        // get the parent goal
        auto& parent_goal = frontier.at(parent_gl);
        
        // otherwise, active eliminate
        parent_goal->candidates.erase(eliminated_rl->idx);

        // if the parent goal has no candidates,
        // return true to indicate conflict
        if (conflict_detector.detect(*parent_goal))
            return true;
    }

    return false;
}
