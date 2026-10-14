#ifndef PUD_BASE_DESCENDER_HPP
#define PUD_BASE_DESCENDER_HPP

#include "value_objects/pud_base_descent.hpp"
#include "value_objects/pud_node_id.hpp"

template<
    typename IRootBindingDescent,
    typename IDescendBinding,
    typename IRootLvcDescent,
    typename IDescendLvc,
    typename IRootBodyGoalExprDescent,
    typename IDescendBodyGoalExpr,
    typename IRootBodyGoalPqDescent,
    typename IDescendBodyGoalPq>
struct pud_base_descender {
    pud_base_descender(
        IRootBindingDescent& root_binding_descent,
        IDescendBinding& descend_binding,
        IRootLvcDescent& root_lvc_descent,
        IDescendLvc& descend_lvc,
        IRootBodyGoalExprDescent& root_body_goal_expr_descent,
        IDescendBodyGoalExpr& descend_body_goal_expr,
        IRootBodyGoalPqDescent& root_body_goal_pq_descent,
        IDescendBodyGoalPq& descend_body_goal_pq);
    pud_base_descent root_descent(pud_node_id root);
    pud_base_descent descend(
        const pud_base_descent& descent,
        pud_node_id parent,
        pud_node_id child);

private:
    IRootBindingDescent& root_binding_descent_;
    IDescendBinding& descend_binding_;
    IRootLvcDescent& root_lvc_descent_;
    IDescendLvc& descend_lvc_;
    IRootBodyGoalExprDescent& root_body_goal_expr_descent_;
    IDescendBodyGoalExpr& descend_body_goal_expr_;
    IRootBodyGoalPqDescent& root_body_goal_pq_descent_;
    IDescendBodyGoalPq& descend_body_goal_pq_;
};

#define PUD_BASE_DESCENDER_TEMPLATE                                                        \
    template<typename IRootBindingDescent, typename IDescendBinding,                       \
             typename IRootLvcDescent, typename IDescendLvc,                               \
             typename IRootBodyGoalExprDescent, typename IDescendBodyGoalExpr,             \
             typename IRootBodyGoalPqDescent, typename IDescendBodyGoalPq>
#define PUD_BASE_DESCENDER                                                                 \
    pud_base_descender<IRootBindingDescent, IDescendBinding, IRootLvcDescent, IDescendLvc, \
        IRootBodyGoalExprDescent, IDescendBodyGoalExpr, IRootBodyGoalPqDescent,            \
        IDescendBodyGoalPq>

PUD_BASE_DESCENDER_TEMPLATE
PUD_BASE_DESCENDER::pud_base_descender(
    IRootBindingDescent& root_binding_descent,
    IDescendBinding& descend_binding,
    IRootLvcDescent& root_lvc_descent,
    IDescendLvc& descend_lvc,
    IRootBodyGoalExprDescent& root_body_goal_expr_descent,
    IDescendBodyGoalExpr& descend_body_goal_expr,
    IRootBodyGoalPqDescent& root_body_goal_pq_descent,
    IDescendBodyGoalPq& descend_body_goal_pq)
    : root_binding_descent_(root_binding_descent)
    , descend_binding_(descend_binding)
    , root_lvc_descent_(root_lvc_descent)
    , descend_lvc_(descend_lvc)
    , root_body_goal_expr_descent_(root_body_goal_expr_descent)
    , descend_body_goal_expr_(descend_body_goal_expr)
    , root_body_goal_pq_descent_(root_body_goal_pq_descent)
    , descend_body_goal_pq_(descend_body_goal_pq) {}

PUD_BASE_DESCENDER_TEMPLATE
pud_base_descent PUD_BASE_DESCENDER::root_descent(pud_node_id root) {
    return pud_base_descent{
        .binding = root_binding_descent_.root_descent(root),
        .lvc = root_lvc_descent_.root_descent(root),
        .body_goal_expr = root_body_goal_expr_descent_.root_descent(root),
        .body_goal_pq = root_body_goal_pq_descent_.root_descent(root),
    };
}

PUD_BASE_DESCENDER_TEMPLATE
pud_base_descent PUD_BASE_DESCENDER::descend(
    const pud_base_descent& descent,
    pud_node_id parent,
    pud_node_id child) {
    return pud_base_descent{
        .binding = descend_binding_.descend(descent.binding, parent, child),
        .lvc = descend_lvc_.descend(descent.lvc, parent, child),
        .body_goal_expr = descend_body_goal_expr_.descend(descent.body_goal_expr, parent, child),
        .body_goal_pq = descend_body_goal_pq_.descend(descent.body_goal_pq, parent, child),
    };
}

#undef PUD_BASE_DESCENDER
#undef PUD_BASE_DESCENDER_TEMPLATE

#endif
