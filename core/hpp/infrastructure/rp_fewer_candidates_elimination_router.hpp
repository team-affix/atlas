#ifndef RP_FEWER_CANDIDATES_ELIMINATION_ROUTER_HPP
#define RP_FEWER_CANDIDATES_ELIMINATION_ROUTER_HPP

#include "value_objects/elimination_result.hpp"
#include "value_objects/lineage.hpp"

template<typename IRouteElimination, typename IComputeActiveGoalValue,
         typename ISetActiveGoalValue>
struct rp_fewer_candidates_elimination_router {
    rp_fewer_candidates_elimination_router(
        IRouteElimination&, IComputeActiveGoalValue&, ISetActiveGoalValue&);
    elimination_result route(const resolution_lineage* rl);
private:
    IRouteElimination& route_elimination_;
    IComputeActiveGoalValue& compute_active_goal_value_;
    ISetActiveGoalValue& set_active_goal_value_;
};

template<typename IRE, typename ICAV, typename ISAGV>
rp_fewer_candidates_elimination_router<IRE, ICAV, ISAGV>::
rp_fewer_candidates_elimination_router(
    IRE& route_elimination, ICAV& compute_active_goal_value,
    ISAGV& set_active_goal_value)
    : route_elimination_(route_elimination)
    , compute_active_goal_value_(compute_active_goal_value)
    , set_active_goal_value_(set_active_goal_value) {}

template<typename IRE, typename ICAV, typename ISAGV>
elimination_result rp_fewer_candidates_elimination_router<IRE, ICAV, ISAGV>::route(
    const resolution_lineage* rl) {
    const elimination_result result = route_elimination_.route(rl);
    if (result != elimination_result::eliminated)
        return result;
    set_active_goal_value_.set_active_goal_value(
        rl->parent, compute_active_goal_value_.compute_active_goal_value(rl->parent));
    return result;
}

#endif
