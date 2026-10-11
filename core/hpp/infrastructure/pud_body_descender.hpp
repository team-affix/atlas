#ifndef PUD_BODY_DESCENDER_HPP
#define PUD_BODY_DESCENDER_HPP

#include <immer/map_transient.hpp>
#include "value_objects/pud_body_descent.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
struct pud_body_descender {
    pud_body_descender(
        IGetNodeAddedBodyGoals& get_node_added_body_goals,
        IGetCallSite& get_call_site);
    pud_body_descent descent_root(pud_node_id root);
    pud_body_descent descend(const pud_body_descent& descent, pud_node_id child);

private:
    void add_body_goals(
        immer::map_transient<body_goal_id, const expr*>& new_body_goals_transient,
        const std::vector<const expr*>& body_goals,
        body_goal_id& next_body_goal_id);

    IGetNodeAddedBodyGoals& get_node_added_body_goals_;
    IGetCallSite& get_call_site_;
};

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
pud_body_descender<IGetNodeAddedBodyGoals, IGetCallSite>::pud_body_descender(
    IGetNodeAddedBodyGoals& get_node_added_body_goals,
    IGetCallSite& get_call_site)
    : get_node_added_body_goals_(get_node_added_body_goals),
      get_call_site_(get_call_site) {
}

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
pud_body_descent pud_body_descender<IGetNodeAddedBodyGoals, IGetCallSite>::descent_root(pud_node_id root) {
    const auto& body_goals = get_node_added_body_goals_.get(root);

    auto new_body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
    
    body_goal_id next_body_goal_id = 0;

    add_body_goals(new_body_goals_transient, body_goals, next_body_goal_id);
    
    return pud_body_descent{
        .body_goals = std::move(new_body_goals_transient).persistent(),
        .next_body_goal_id = next_body_goal_id,
    };
}

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
pud_body_descent pud_body_descender<IGetNodeAddedBodyGoals, IGetCallSite>::descend(const pud_body_descent& descent, pud_node_id child) {
    const auto& body_goals = get_node_added_body_goals_.get(child);

    const body_goal_id call_site = get_call_site_.get(child);

    auto new_body_goals_transient = descent.body_goals.transient();

    // erase the call site body goal from the current node (since it has been resolved)
    new_body_goals_transient.erase(call_site);
    
    body_goal_id next_body_goal_id = descent.next_body_goal_id;

    add_body_goals(new_body_goals_transient, body_goals, next_body_goal_id);

    return pud_body_descent{
        .body_goals = std::move(new_body_goals_transient).persistent(),
        .next_body_goal_id = next_body_goal_id,
    };
}

#endif
