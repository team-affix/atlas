#ifndef DBUCT_LEARN_REAPPLY_HPP
#define DBUCT_LEARN_REAPPLY_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/elimination_result.hpp"

// Re-applies learned avoidances to the current (post-backtrack) frontier. After
// DBUCT unwinds to a resume node, that frontier lacks the eliminations discovered
// deeper in the tree; reapply_frontier() re-derives and routes every forced
// elimination and reports whether the frontier collapsed. The solver drives this
// to a fixpoint across levels, which is how cascading backjumps arise.
template<typename ICdclReapply, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
struct dbuct_learn_reapply {
    dbuct_learn_reapply(ICdclReapply& cdcl, IEliminationRouter& router,
                        IConflictDetector& conflict_detector,
                        IUnitGoalDetector& unit_goal_detector,
                        IPushUnitGoal& push_unit_goal);

    bool route_elimination(const resolution_lineage* rl);

    bool reapply_frontier();

private:
    ICdclReapply& cdcl_;
    IEliminationRouter& router_;
    IConflictDetector& conflict_detector_;
    IUnitGoalDetector& unit_goal_detector_;
    IPushUnitGoal& push_unit_goal_;
};

template<typename ICdclReapply, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
dbuct_learn_reapply<ICdclReapply, IEliminationRouter, IConflictDetector,
                    IUnitGoalDetector, IPushUnitGoal>::dbuct_learn_reapply(
    ICdclReapply& cdcl, IEliminationRouter& router, IConflictDetector& conflict_detector,
    IUnitGoalDetector& unit_goal_detector, IPushUnitGoal& push_unit_goal)
    : cdcl_(cdcl), router_(router), conflict_detector_(conflict_detector),
      unit_goal_detector_(unit_goal_detector), push_unit_goal_(push_unit_goal) {}

template<typename ICdclReapply, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
bool dbuct_learn_reapply<ICdclReapply, IEliminationRouter, IConflictDetector,
                         IUnitGoalDetector, IPushUnitGoal>::route_elimination(const resolution_lineage* rl) {
    if (router_.route(rl) != elimination_result::eliminated)
        return false;
    const goal_lineage* gl = rl->parent;
    if (conflict_detector_.detect(gl))
        return true;
    if (unit_goal_detector_.detect(gl))
        push_unit_goal_.push(gl);
    return false;
}

template<typename ICdclReapply, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
bool dbuct_learn_reapply<ICdclReapply, IEliminationRouter, IConflictDetector,
                         IUnitGoalDetector, IPushUnitGoal>::reapply_frontier() {
    bool conflict = false;
    for (const resolution_lineage* forced : cdcl_.reapply()) {
        if (route_elimination(forced))
            conflict = true;
    }
    if (cdcl_.reapply_found_realized_conflict())
        conflict = true;
    return conflict;
}

#endif
