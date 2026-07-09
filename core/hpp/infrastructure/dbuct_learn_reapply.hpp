#ifndef DBUCT_LEARN_REAPPLY_HPP
#define DBUCT_LEARN_REAPPLY_HPP

#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/elimination_result.hpp"

// Routes the eliminations CDCL emits from pop_frame during backtracking. These
// forced eliminations bypass the joint eliminator (they surface from the frame
// pops in dbuct_sim::terminate, not from constrain), so this router carries the
// same obligations the joint applies to CDCL constrain hits: drop the lineage's
// MHU head, deactivate the candidate through the shared elimination router, and
// -- when a goal fully collapses -- fire conflict / unit-goal detection. The
// solver cascades on the returned "a conflict was realized" signal.
template<typename IMhu, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
struct dbuct_learn_reapply {
    dbuct_learn_reapply(IMhu& mhu, IEliminationRouter& router,
                        IConflictDetector& conflict_detector,
                        IUnitGoalDetector& unit_goal_detector,
                        IPushUnitGoal& push_unit_goal);

    bool route_elimination(const resolution_lineage* rl);
    bool route_eliminations(const std::vector<const resolution_lineage*>& elims);

private:
    IMhu& mhu_;
    IEliminationRouter& router_;
    IConflictDetector& conflict_detector_;
    IUnitGoalDetector& unit_goal_detector_;
    IPushUnitGoal& push_unit_goal_;
};

template<typename IMhu, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
dbuct_learn_reapply<IMhu, IEliminationRouter, IConflictDetector,
                    IUnitGoalDetector, IPushUnitGoal>::dbuct_learn_reapply(
    IMhu& mhu, IEliminationRouter& router, IConflictDetector& conflict_detector,
    IUnitGoalDetector& unit_goal_detector, IPushUnitGoal& push_unit_goal)
    : mhu_(mhu), router_(router), conflict_detector_(conflict_detector),
      unit_goal_detector_(unit_goal_detector), push_unit_goal_(push_unit_goal) {}

template<typename IMhu, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
bool dbuct_learn_reapply<IMhu, IEliminationRouter, IConflictDetector,
                         IUnitGoalDetector, IPushUnitGoal>::route_elimination(const resolution_lineage* rl) {
    mhu_.remove_head(rl);
    const elimination_result res = router_.route(rl);
    if (res != elimination_result::eliminated)
        return false;
    const goal_lineage* gl = rl->parent;
    if (conflict_detector_.detect(gl))
        return true;
    if (unit_goal_detector_.detect(gl))
        push_unit_goal_.push(gl);
    return false;
}

template<typename IMhu, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
bool dbuct_learn_reapply<IMhu, IEliminationRouter, IConflictDetector,
                         IUnitGoalDetector, IPushUnitGoal>::route_eliminations(
    const std::vector<const resolution_lineage*>& elims) {
    bool conflict = false;
    for (const resolution_lineage* rl : elims) {
        if (route_elimination(rl))
            conflict = true;
    }
    return conflict;
}

#endif
