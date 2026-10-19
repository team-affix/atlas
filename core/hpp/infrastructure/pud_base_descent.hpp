#ifndef PUD_BASE_DESCENT_HPP
#define PUD_BASE_DESCENT_HPP

#include <utility>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

template<
    typename BindingDescent,
    typename LvcDescent,
    typename NextBodyGoalIdDescent,
    typename BodyGoalExprDescent,
    typename StructureWatchDescent,
    typename BodyGoalPqDescent>
struct pud_base_descent {
    pud_base_descent(
        BindingDescent binding,
        LvcDescent lvc,
        NextBodyGoalIdDescent next_body_goal_id,
        BodyGoalExprDescent body_goal_expr,
        StructureWatchDescent structure_watch,
        BodyGoalPqDescent body_goal_pq);
    pud_base_descent descend(pud_node_id parent, pud_node_id child) const;
    const BindingDescent& binding() const;
    const LvcDescent& lvc() const;
    const BodyGoalExprDescent& body_goal_expr() const;
    const BodyGoalPqDescent& body_goal_pq() const;
private:
    BindingDescent binding_;
    LvcDescent lvc_;
    NextBodyGoalIdDescent next_body_goal_id_;
    BodyGoalExprDescent body_goal_expr_;
    StructureWatchDescent structure_watch_;
    BodyGoalPqDescent body_goal_pq_;
};

#define PUD_BASE_DESCENT_TEMPLATE                                                          \
    template<typename BindingDescent, typename LvcDescent, typename NextBodyGoalIdDescent, \
             typename BodyGoalExprDescent, typename StructureWatchDescent,                 \
             typename BodyGoalPqDescent>
#define PUD_BASE_DESCENT                                                                   \
    pud_base_descent<BindingDescent, LvcDescent, NextBodyGoalIdDescent, BodyGoalExprDescent, \
        StructureWatchDescent, BodyGoalPqDescent>

PUD_BASE_DESCENT_TEMPLATE
PUD_BASE_DESCENT::pud_base_descent(
    BindingDescent binding,
    LvcDescent lvc,
    NextBodyGoalIdDescent next_body_goal_id,
    BodyGoalExprDescent body_goal_expr,
    StructureWatchDescent structure_watch,
    BodyGoalPqDescent body_goal_pq)
    : binding_(std::move(binding))
    , lvc_(std::move(lvc))
    , next_body_goal_id_(std::move(next_body_goal_id))
    , body_goal_expr_(std::move(body_goal_expr))
    , structure_watch_(std::move(structure_watch))
    , body_goal_pq_(std::move(body_goal_pq)) {}

PUD_BASE_DESCENT_TEMPLATE
PUD_BASE_DESCENT PUD_BASE_DESCENT::descend(pud_node_id parent, pud_node_id child) const {
    const pud_body_goal_id next_body_goal_id = next_body_goal_id_.next_body_goal_id();
    BindingDescent child_binding = binding_.descend(child);
    LvcDescent child_lvc = lvc_.descend(child);
    NextBodyGoalIdDescent child_next_body_goal_id = next_body_goal_id_.descend(child);
    BodyGoalExprDescent child_body_goal_expr = body_goal_expr_.descend(next_body_goal_id, parent, child);

    std::vector<const expr*> watch;
    for (pud_body_goal_id goal = next_body_goal_id; goal < child_next_body_goal_id.next_body_goal_id(); ++goal)
        watch.push_back(child_body_goal_expr.body_goal_exprs().at(goal));
    auto watched = structure_watch_.descend(child, {body_goal_pq_.find_call_site_watch_head_id(parent)}, watch);

    BodyGoalPqDescent child_body_goal_pq = body_goal_pq_.descend(
        next_body_goal_id,
        parent,
        child,
        child_binding.ubs_state(),
        child_lvc.lvc(),
        child_body_goal_expr.body_goal_exprs(),
        watched.structure_changes,
        watched.added_head_ids);

    return PUD_BASE_DESCENT{
        std::move(child_binding),
        std::move(child_lvc),
        std::move(child_next_body_goal_id),
        std::move(child_body_goal_expr),
        std::move(watched.child_descent),
        std::move(child_body_goal_pq),
    };
}

PUD_BASE_DESCENT_TEMPLATE
const BindingDescent& PUD_BASE_DESCENT::binding() const {
    return binding_;
}

PUD_BASE_DESCENT_TEMPLATE
const LvcDescent& PUD_BASE_DESCENT::lvc() const {
    return lvc_;
}

PUD_BASE_DESCENT_TEMPLATE
const BodyGoalExprDescent& PUD_BASE_DESCENT::body_goal_expr() const {
    return body_goal_expr_;
}

PUD_BASE_DESCENT_TEMPLATE
const BodyGoalPqDescent& PUD_BASE_DESCENT::body_goal_pq() const {
    return body_goal_pq_;
}

#undef PUD_BASE_DESCENT
#undef PUD_BASE_DESCENT_TEMPLATE

#endif
