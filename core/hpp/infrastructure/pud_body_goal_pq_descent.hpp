#ifndef PUD_BODY_GOAL_PQ_DESCENT_HPP
#define PUD_BODY_GOAL_PQ_DESCENT_HPP

#include <cstddef>
#include <vector>
#include <immer/box.hpp>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/persistent_query.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
struct pud_body_goal_pq_descent {
    pud_body_goal_pq_descent(
        IGetNodeAddedBodyGoals& get_node_added_body_goals,
        IGetCallSite& get_call_site,
        pud_node_id root);
    pud_body_goal_pq_descent descend(
        body_goal_id next_body_goal_id,
        pud_node_id parent,
        pud_node_id child) const;
    const immer::map<body_goal_id, immer::box<persistent_query>>& body_goal_pqs() const;
private:
    using map_t = immer::map<body_goal_id, immer::box<persistent_query>>;

    static void add_body_goals(
        map_t::transient_type& body_goal_pqs_transient,
        const std::vector<const expr*>& body_goals,
        body_goal_id next_body_goal_id);

    IGetNodeAddedBodyGoals* get_node_added_body_goals_;
    IGetCallSite* get_call_site_;
    map_t body_goal_pqs_;
};

#define PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE                                                  \
    template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
#define PUD_BODY_GOAL_PQ_DESCENT                                                           \
    pud_body_goal_pq_descent<IGetNodeAddedBodyGoals, IGetCallSite>

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
PUD_BODY_GOAL_PQ_DESCENT::pud_body_goal_pq_descent(
    IGetNodeAddedBodyGoals& get_node_added_body_goals,
    IGetCallSite& get_call_site,
    pud_node_id root)
    : get_node_added_body_goals_(&get_node_added_body_goals)
    , get_call_site_(&get_call_site)
    , body_goal_pqs_() {
    auto body_goal_pqs_transient = body_goal_pqs_.transient();
    add_body_goals(body_goal_pqs_transient, get_node_added_body_goals_->get(root), 0);
    body_goal_pqs_ = std::move(body_goal_pqs_transient).persistent();
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
PUD_BODY_GOAL_PQ_DESCENT PUD_BODY_GOAL_PQ_DESCENT::descend(
    body_goal_id next_body_goal_id,
    pud_node_id parent,
    pud_node_id child) const {
    auto body_goal_pqs_transient = body_goal_pqs_.transient();
    body_goal_pqs_transient.erase(get_call_site_->get(parent));
    add_body_goals(body_goal_pqs_transient, get_node_added_body_goals_->get(child), next_body_goal_id);
    PUD_BODY_GOAL_PQ_DESCENT result = *this;
    result.body_goal_pqs_ = std::move(body_goal_pqs_transient).persistent();
    return result;
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
const immer::map<body_goal_id, immer::box<persistent_query>>& PUD_BODY_GOAL_PQ_DESCENT::body_goal_pqs() const {
    return body_goal_pqs_;
}

PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE
void PUD_BODY_GOAL_PQ_DESCENT::add_body_goals(
    map_t::transient_type& body_goal_pqs_transient,
    const std::vector<const expr*>& body_goals,
    body_goal_id next_body_goal_id) {
    for (size_t body_goal_index = 0; body_goal_index < body_goals.size(); ++body_goal_index)
        body_goal_pqs_transient.set(next_body_goal_id + body_goal_index, immer::box<persistent_query>{});
}

#undef PUD_BODY_GOAL_PQ_DESCENT
#undef PUD_BODY_GOAL_PQ_DESCENT_TEMPLATE

#endif
