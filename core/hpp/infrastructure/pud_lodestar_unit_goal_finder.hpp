#ifndef PUD_LODESTAR_UNIT_GOAL_FINDER_HPP
#define PUD_LODESTAR_UNIT_GOAL_FINDER_HPP

#include <optional>
#include <set>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_query_introduction.hpp"

// Keeps the queries that went unit, as the MHQ reports them, and finds a leaf's unit goal among them. A unit query
// pulls no unfold by itself: it only decides the goal once the leaf holding it is chosen. A query that was removed
// or no longer has exactly one candidate is dropped the next time it is looked at.
template<typename IFindQueryIntroduction, typename IGetQueryCandidates, typename IFindNodeQueries>
struct pud_lodestar_unit_goal_finder {
    pud_lodestar_unit_goal_finder(
        IFindQueryIntroduction& find_query_introduction,
        IGetQueryCandidates& get_query_candidates,
        IFindNodeQueries& find_node_queries);
    void push_back(pud_mhq_head_id head_id);
    std::optional<pud_body_goal_id> find_unit_goal(pud_global_node_id leaf);
private:
    IFindQueryIntroduction& find_query_introduction_;
    IGetQueryCandidates& get_query_candidates_;
    IFindNodeQueries& find_node_queries_;
    std::set<pud_mhq_head_id> unit_queries_;
};

#define PUD_LODESTAR_UNIT_GOAL_FINDER_TEMPLATE                                                         \
    template<typename IFindQueryIntroduction, typename IGetQueryCandidates, typename IFindNodeQueries>
#define PUD_LODESTAR_UNIT_GOAL_FINDER                                                                  \
    pud_lodestar_unit_goal_finder<IFindQueryIntroduction, IGetQueryCandidates, IFindNodeQueries>

PUD_LODESTAR_UNIT_GOAL_FINDER_TEMPLATE
PUD_LODESTAR_UNIT_GOAL_FINDER::pud_lodestar_unit_goal_finder(
    IFindQueryIntroduction& find_query_introduction,
    IGetQueryCandidates& get_query_candidates,
    IFindNodeQueries& find_node_queries)
    : find_query_introduction_(find_query_introduction)
    , get_query_candidates_(get_query_candidates)
    , find_node_queries_(find_node_queries)
    , unit_queries_() {}

PUD_LODESTAR_UNIT_GOAL_FINDER_TEMPLATE
void PUD_LODESTAR_UNIT_GOAL_FINDER::push_back(pud_mhq_head_id head_id) {
    unit_queries_.insert(head_id);
}

PUD_LODESTAR_UNIT_GOAL_FINDER_TEMPLATE
std::optional<pud_body_goal_id> PUD_LODESTAR_UNIT_GOAL_FINDER::find_unit_goal(pud_global_node_id leaf) {
    const auto* queries = find_node_queries_.find(leaf);
    std::optional<pud_body_goal_id> smallest_unit_goal;
    for (auto unit = unit_queries_.begin(); unit != unit_queries_.end();) {
        const pud_mhq_head_id head_id = *unit;
        const std::optional<pud_query_introduction> introduction = find_query_introduction_.find_introduction(head_id);
        const bool still_unit = introduction.has_value() && get_query_candidates_.candidates(head_id).size() == 1;
        if (!still_unit) {
            unit = unit_queries_.erase(unit);
            continue;
        }
        ++unit;
        if (queries == nullptr)
            continue;
        const pud_body_goal_id goal = introduction->goal;
        const auto* owner = queries->find(goal);
        const bool held = owner != nullptr && (*owner)->head_id() == head_id;
        if (!held)
            continue;
        if (smallest_unit_goal.has_value() && goal >= smallest_unit_goal.value())
            continue;
        smallest_unit_goal = goal;
    }
    return smallest_unit_goal;
}

#undef PUD_LODESTAR_UNIT_GOAL_FINDER
#undef PUD_LODESTAR_UNIT_GOAL_FINDER_TEMPLATE

#endif
