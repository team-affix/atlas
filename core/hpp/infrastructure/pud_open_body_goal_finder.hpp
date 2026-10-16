#ifndef PUD_OPEN_BODY_GOAL_FINDER_HPP
#define PUD_OPEN_BODY_GOAL_FINDER_HPP

#include <map>
#include <vector>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

// The body goals still open at the end of a root-first path, in body goal id order.
template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
struct pud_open_body_goal_finder {
    pud_open_body_goal_finder(IGetNodeAddedBodyGoals& get_node_added_body_goals, IGetCallSite& get_call_site);
    std::vector<const expr*> find_open_body_goals(const std::vector<pud_node_id>& path) const;
private:
    IGetNodeAddedBodyGoals& get_node_added_body_goals_;
    IGetCallSite& get_call_site_;
};

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
pud_open_body_goal_finder<IGetNodeAddedBodyGoals, IGetCallSite>::pud_open_body_goal_finder(
    IGetNodeAddedBodyGoals& get_node_added_body_goals,
    IGetCallSite& get_call_site)
    : get_node_added_body_goals_(get_node_added_body_goals)
    , get_call_site_(get_call_site) {}

template<typename IGetNodeAddedBodyGoals, typename IGetCallSite>
std::vector<const expr*> pud_open_body_goal_finder<IGetNodeAddedBodyGoals, IGetCallSite>::find_open_body_goals(
    const std::vector<pud_node_id>& path) const {
    std::map<body_goal_id, const expr*> open_body_goals;
    body_goal_id next_body_goal_id = 0;
    for (size_t path_index = 0; path_index < path.size(); ++path_index) {
        if (path_index > 0)
            open_body_goals.erase(get_call_site_.get(path.at(path_index - 1)));
        for (const expr* body_goal : get_node_added_body_goals_.get(path.at(path_index)))
            open_body_goals.emplace(next_body_goal_id++, body_goal);
    }
    std::vector<const expr*> result;
    result.reserve(open_body_goals.size());
    for (const auto& [goal, goal_expr] : open_body_goals)
        result.push_back(goal_expr);
    return result;
}

#endif
