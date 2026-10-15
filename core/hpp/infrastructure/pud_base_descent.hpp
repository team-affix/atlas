#ifndef PUD_BASE_DESCENT_HPP
#define PUD_BASE_DESCENT_HPP

#include <utility>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"

template<
    typename BindingDescent,
    typename LvcDescent,
    typename NextBodyGoalIdDescent,
    typename BodyGoalExprDescent,
    typename BodyGoalPqDescent>
struct pud_base_descent {
    pud_base_descent(
        BindingDescent binding,
        LvcDescent lvc,
        NextBodyGoalIdDescent next_body_goal_id,
        BodyGoalExprDescent body_goal_expr,
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
    BodyGoalPqDescent body_goal_pq_;
};

#define PUD_BASE_DESCENT_TEMPLATE                                                          \
    template<typename BindingDescent, typename LvcDescent, typename NextBodyGoalIdDescent, \
             typename BodyGoalExprDescent, typename BodyGoalPqDescent>
#define PUD_BASE_DESCENT                                                                   \
    pud_base_descent<BindingDescent, LvcDescent, NextBodyGoalIdDescent,                    \
        BodyGoalExprDescent, BodyGoalPqDescent>

PUD_BASE_DESCENT_TEMPLATE
PUD_BASE_DESCENT::pud_base_descent(
    BindingDescent binding,
    LvcDescent lvc,
    NextBodyGoalIdDescent next_body_goal_id,
    BodyGoalExprDescent body_goal_expr,
    BodyGoalPqDescent body_goal_pq)
    : binding_(std::move(binding))
    , lvc_(std::move(lvc))
    , next_body_goal_id_(std::move(next_body_goal_id))
    , body_goal_expr_(std::move(body_goal_expr))
    , body_goal_pq_(std::move(body_goal_pq)) {}

PUD_BASE_DESCENT_TEMPLATE
PUD_BASE_DESCENT PUD_BASE_DESCENT::descend(pud_node_id parent, pud_node_id child) const {
    const body_goal_id next_body_goal_id = next_body_goal_id_.next_body_goal_id();
    return PUD_BASE_DESCENT{
        binding_.descend(child),
        lvc_.descend(child),
        next_body_goal_id_.descend(child),
        body_goal_expr_.descend(next_body_goal_id, parent, child),
        body_goal_pq_.descend(next_body_goal_id, parent, child),
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
