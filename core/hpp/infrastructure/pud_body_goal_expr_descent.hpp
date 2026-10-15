#ifndef PUD_BODY_GOAL_EXPR_DESCENT_HPP
#define PUD_BODY_GOAL_EXPR_DESCENT_HPP

#include <cstddef>
#include <vector>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
struct pud_body_goal_expr_descent {
    pud_body_goal_expr_descent(
        IGetNodeAddedBodyGoals& get_node_added_body_goals,
        IGetCallSite& get_call_site,
        pud_node_id root);
    pud_body_goal_expr_descent descend(
        body_goal_id next_body_goal_id,
        pud_node_id parent,
        pud_node_id child) const;
    const immer::map<body_goal_id, const expr*>& body_goal_exprs() const;
private:
    using map_t = immer::map<body_goal_id, const expr*>;

    void add_body_goals(
        map_t::transient_type& body_goal_exprs_transient,
        const std::vector<const expr*>& body_goals,
        body_goal_id next_body_goal_id) const;

    IGetNodeAddedBodyGoals* get_node_added_body_goals_;
    IGetCallSite* get_call_site_;
    map_t body_goal_exprs_;
};

#define PUD_BODY_GOAL_EXPR_DESCENT_TEMPLATE                                                \
    template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
#define PUD_BODY_GOAL_EXPR_DESCENT                                                         \
    pud_body_goal_expr_descent<IGetNodeAddedBodyGoals, IGetCallSite>

PUD_BODY_GOAL_EXPR_DESCENT_TEMPLATE
PUD_BODY_GOAL_EXPR_DESCENT::pud_body_goal_expr_descent(
    IGetNodeAddedBodyGoals& get_node_added_body_goals,
    IGetCallSite& get_call_site,
    pud_node_id root)
    : get_node_added_body_goals_(&get_node_added_body_goals)
    , get_call_site_(&get_call_site)
    , body_goal_exprs_() {
    auto body_goal_exprs_transient = body_goal_exprs_.transient();
    add_body_goals(body_goal_exprs_transient, get_node_added_body_goals_->get(root), 0);
    body_goal_exprs_ = std::move(body_goal_exprs_transient).persistent();
}

PUD_BODY_GOAL_EXPR_DESCENT_TEMPLATE
PUD_BODY_GOAL_EXPR_DESCENT PUD_BODY_GOAL_EXPR_DESCENT::descend(
    body_goal_id next_body_goal_id,
    pud_node_id parent,
    pud_node_id child) const {
    auto body_goal_exprs_transient = body_goal_exprs_.transient();
    body_goal_exprs_transient.erase(get_call_site_->get(parent));
    add_body_goals(body_goal_exprs_transient, get_node_added_body_goals_->get(child), next_body_goal_id);
    PUD_BODY_GOAL_EXPR_DESCENT result = *this;
    result.body_goal_exprs_ = std::move(body_goal_exprs_transient).persistent();
    return result;
}

PUD_BODY_GOAL_EXPR_DESCENT_TEMPLATE
const immer::map<body_goal_id, const expr*>& PUD_BODY_GOAL_EXPR_DESCENT::body_goal_exprs() const {
    return body_goal_exprs_;
}

PUD_BODY_GOAL_EXPR_DESCENT_TEMPLATE
void PUD_BODY_GOAL_EXPR_DESCENT::add_body_goals(
    map_t::transient_type& body_goal_exprs_transient,
    const std::vector<const expr*>& body_goals,
    body_goal_id next_body_goal_id) const {
    for (size_t body_goal_index = 0; body_goal_index < body_goals.size(); ++body_goal_index)
        body_goal_exprs_transient.set(next_body_goal_id + body_goal_index, body_goals.at(body_goal_index));
}

#undef PUD_BODY_GOAL_EXPR_DESCENT
#undef PUD_BODY_GOAL_EXPR_DESCENT_TEMPLATE

#endif
