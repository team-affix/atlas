#ifndef PUD_BODY_GOAL_EXPR_DESCENDER_HPP
#define PUD_BODY_GOAL_EXPR_DESCENDER_HPP

#include <vector>
#include <immer/map_transient.hpp>
#include "value_objects/pud_body_goal_expr_descent.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
struct pud_body_goal_expr_descender {
    pud_body_goal_expr_descender(
        IGetNodeAddedBodyGoals& get_node_added_body_goals,
        IGetCallSite& get_call_site);
    pud_body_goal_expr_descent root_descent(pud_node_id root);
    pud_body_goal_expr_descent descend(
        const pud_body_goal_expr_descent& descent,
        pud_node_id parent,
        pud_node_id child);

private:
    void add_body_goals(
        immer::map<body_goal_id, const expr*>::transient_type& body_goal_exprs_transient,
        const std::vector<const expr*>& body_goals,
        body_goal_id& next_body_goal_id);

    IGetNodeAddedBodyGoals& get_node_added_body_goals_;
    IGetCallSite& get_call_site_;
};

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
pud_body_goal_expr_descender<IGetNodeAddedBodyGoals, IGetCallSite>::pud_body_goal_expr_descender(
    IGetNodeAddedBodyGoals& get_node_added_body_goals,
    IGetCallSite& get_call_site)
    : get_node_added_body_goals_(get_node_added_body_goals)
    , get_call_site_(get_call_site) {}

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
void pud_body_goal_expr_descender<IGetNodeAddedBodyGoals, IGetCallSite>::add_body_goals(
    immer::map<body_goal_id, const expr*>::transient_type& body_goal_exprs_transient,
    const std::vector<const expr*>& body_goals,
    body_goal_id& next_body_goal_id) {
    for (const expr* goal : body_goals) {
        body_goal_exprs_transient.set(next_body_goal_id, goal);
        ++next_body_goal_id;
    }
}

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
pud_body_goal_expr_descent pud_body_goal_expr_descender<IGetNodeAddedBodyGoals, IGetCallSite>::root_descent(
    pud_node_id root) {
    const std::vector<const expr*>& body_goals = get_node_added_body_goals_.get(root);
    auto body_goal_exprs_transient = immer::map<body_goal_id, const expr*>{}.transient();
    body_goal_id next_body_goal_id = 0;
    add_body_goals(body_goal_exprs_transient, body_goals, next_body_goal_id);
    return pud_body_goal_expr_descent{
        .body_goal_exprs = std::move(body_goal_exprs_transient).persistent(),
        .next_body_goal_id = next_body_goal_id,
    };
}

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
pud_body_goal_expr_descent pud_body_goal_expr_descender<IGetNodeAddedBodyGoals, IGetCallSite>::descend(
    const pud_body_goal_expr_descent& descent,
    pud_node_id parent,
    pud_node_id child) {
    const std::vector<const expr*>& body_goals = get_node_added_body_goals_.get(child);
    auto body_goal_exprs_transient = descent.body_goal_exprs.transient();
    const body_goal_id call_site = get_call_site_.get(parent);
    body_goal_exprs_transient.erase(call_site);
    body_goal_id next_body_goal_id = descent.next_body_goal_id;
    add_body_goals(body_goal_exprs_transient, body_goals, next_body_goal_id);
    return pud_body_goal_expr_descent{
        .body_goal_exprs = std::move(body_goal_exprs_transient).persistent(),
        .next_body_goal_id = next_body_goal_id,
    };
}

#endif
