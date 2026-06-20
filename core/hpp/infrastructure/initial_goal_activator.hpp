#ifndef INITIAL_GOAL_ACTIVATOR_HPP
#define INITIAL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IInitialGoalExprs, typename IMakeInitialGoalLineage,
         typename IGoalExprs, typename IGoalCandidateRules, typename IActiveGoals>
struct initial_goal_activator {
    initial_goal_activator(IInitialGoalExprs&, IMakeInitialGoalLineage&,
                           IGoalExprs&, IGoalCandidateRules&, IActiveGoals&);
    void activate_initial_goal(subgoal_id idx);
private:
    IInitialGoalExprs& get_initial_goal_expr;
    IMakeInitialGoalLineage& make_initial_goal_lineage;
    IGoalExprs& set_goal_expr;
    IGoalCandidateRules& insert_goal_candidates;
    IActiveGoals& insert_active_goal;
};

template<typename IIGE, typename IMIGL, typename IGE, typename IGCR, typename IAG>
initial_goal_activator<IIGE, IMIGL, IGE, IGCR, IAG>::initial_goal_activator(
    IIGE& ige, IMIGL& migl, IGE& ge, IGCR& gcr, IAG& ag)
    : get_initial_goal_expr(ige), make_initial_goal_lineage(migl),
      set_goal_expr(ge), insert_goal_candidates(gcr), insert_active_goal(ag) {}

template<typename IIGE, typename IMIGL, typename IGE, typename IGCR, typename IAG>
void initial_goal_activator<IIGE, IMIGL, IGE, IGCR, IAG>::activate_initial_goal(
    subgoal_id idx) {
    const expr* goal_expr = get_initial_goal_expr.get(idx);
    const goal_lineage* gl = make_initial_goal_lineage.make(idx);
    set_goal_expr.set(gl, framed_expr{goal_expr, 0});
    insert_goal_candidates.insert(gl);
    insert_active_goal.insert_active_goal(gl);
}

#endif
