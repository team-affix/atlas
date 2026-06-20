#ifndef INITIAL_GOAL_ACTIVATOR_HPP
#define INITIAL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IGetInitialGoalExpr, typename IMakeInitialGoalLineage,
         typename ISetGoalExpr, typename IInsertGoalCandidates, typename IInsertActiveGoal>
struct initial_goal_activator {
    initial_goal_activator(IGetInitialGoalExpr&, IMakeInitialGoalLineage&,
                           ISetGoalExpr&, IInsertGoalCandidates&, IInsertActiveGoal&);
    void activate_initial_goal(subgoal_id idx);
private:
    IGetInitialGoalExpr& get_initial_goal_expr;
    IMakeInitialGoalLineage& make_initial_goal_lineage;
    ISetGoalExpr& set_goal_expr;
    IInsertGoalCandidates& insert_goal_candidates;
    IInsertActiveGoal& insert_active_goal;
};

template<typename IGIGE, typename IMIGL, typename ISGE, typename IIGC, typename IIAG>
initial_goal_activator<IGIGE, IMIGL, ISGE, IIGC, IIAG>::initial_goal_activator(
    IGIGE& ige, IMIGL& migl, ISGE& ge, IIGC& gcr, IIAG& ag)
    : get_initial_goal_expr(ige), make_initial_goal_lineage(migl),
      set_goal_expr(ge), insert_goal_candidates(gcr), insert_active_goal(ag) {}

template<typename IGIGE, typename IMIGL, typename ISGE, typename IIGC, typename IIAG>
void initial_goal_activator<IGIGE, IMIGL, ISGE, IIGC, IIAG>::activate_initial_goal(
    subgoal_id idx) {
    const expr* goal_expr = get_initial_goal_expr.get(idx);
    const goal_lineage* gl = make_initial_goal_lineage.make(idx);
    set_goal_expr.set(gl, framed_expr{goal_expr, 0});
    insert_goal_candidates.insert(gl);
    insert_active_goal.insert_active_goal(gl);
}

#endif
