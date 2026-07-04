#ifndef DBUCT_LEARN_REAPPLY_HPP
#define DBUCT_LEARN_REAPPLY_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/elimination_result.hpp"

// Re-applies learned avoidances to the current (post-backtrack) frontier.
//
// This is the counterpart to the plan's "backtrack, then apply updates from
// things learned deeper in the tree". After DBUCT unwinds to a resume node, the
// solver state is exactly the frontier that existed there before we ever camped
// deeper — so it has none of the eliminations that conflict analysis discovered
// below. reapply_frontier() re-derives every forced elimination for that frontier
// and routes it, reporting whether the frontier has collapsed (an active goal
// lost all candidates, or the frontier already realises a full learned no-good).
//
// The solver drives this to a fixpoint across backtrack levels: a collapse here
// triggers a further lazy backstep and another re-application at the shallower
// frontier, which is how cascading backjumps arise.
template<typename ICdclReapply, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
struct dbuct_learn_reapply {
    dbuct_learn_reapply(ICdclReapply& cdcl, IEliminationRouter& router,
                        IConflictDetector& conflict_detector,
                        IUnitGoalDetector& unit_goal_detector,
                        IPushUnitGoal& push_unit_goal)
        : cdcl_(cdcl), router_(router), conflict_detector_(conflict_detector),
          unit_goal_detector_(unit_goal_detector), push_unit_goal_(push_unit_goal) {}

    // Route a single elimination (e.g. a freshly learned unit no-good). Returns
    // true if it emptied the active candidate set of its goal.
    bool route_elimination(const resolution_lineage* rl) {
        if (router_.route(rl) != elimination_result::eliminated)
            return false;
        const goal_lineage* gl = rl->parent;
        if (conflict_detector_.detect(gl))
            return true;
        if (unit_goal_detector_.detect(gl))
            push_unit_goal_.push(gl);
        return false;
    }

    // Re-derive and route all forced eliminations for the current frontier.
    // Returns true if the frontier is conflicted.
    bool reapply_frontier() {
        bool conflict = false;
        auto sm = cdcl_.reapply();
        while (!sm.done()) {
            sm.resume();
            if (!sm.has_yield())
                continue;
            if (route_elimination(sm.consume_yield()))
                conflict = true;
        }
        if (cdcl_.reapply_found_realized_conflict())
            conflict = true;
        return conflict;
    }

private:
    ICdclReapply& cdcl_;
    IEliminationRouter& router_;
    IConflictDetector& conflict_detector_;
    IUnitGoalDetector& unit_goal_detector_;
    IPushUnitGoal& push_unit_goal_;
};

#endif
