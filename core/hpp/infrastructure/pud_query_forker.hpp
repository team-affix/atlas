#ifndef PUD_QUERY_FORKER_HPP
#define PUD_QUERY_FORKER_HPP

#include <optional>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Forks a query a leaf inherited, for a goal whose structure changed, into the leaf's environment.
// None when no candidate of the inherited query survives the fork.
template<typename IForkQuery, typename IFindNodeGoal, typename IGetNodeLvc>
struct pud_query_forker {
    pud_query_forker(IForkQuery& fork_query, IFindNodeGoal& find_node_goal, IGetNodeLvc& get_node_lvc);
    std::optional<pud_mhq_head_id> fork(
        pud_mhq_head_id inherited,
        pud_node_id leaf,
        const pud_ubs_state& bindings,
        pud_body_goal_id goal);
private:
    IForkQuery& fork_query_;
    IFindNodeGoal& find_node_goal_;
    IGetNodeLvc& get_node_lvc_;
};

#define PUD_QUERY_FORKER_TEMPLATE template<typename IForkQuery, typename IFindNodeGoal, typename IGetNodeLvc>
#define PUD_QUERY_FORKER pud_query_forker<IForkQuery, IFindNodeGoal, IGetNodeLvc>

PUD_QUERY_FORKER_TEMPLATE
PUD_QUERY_FORKER::pud_query_forker(IForkQuery& fork_query, IFindNodeGoal& find_node_goal, IGetNodeLvc& get_node_lvc)
    : fork_query_(fork_query)
    , find_node_goal_(find_node_goal)
    , get_node_lvc_(get_node_lvc) {}

PUD_QUERY_FORKER_TEMPLATE
std::optional<pud_mhq_head_id> PUD_QUERY_FORKER::fork(
    pud_mhq_head_id inherited,
    pud_node_id leaf,
    const pud_ubs_state& bindings,
    pud_body_goal_id goal) {
    return fork_query_.try_fork(inherited, bindings, find_node_goal_.find(leaf, goal), get_node_lvc_.get(leaf));
}

#undef PUD_QUERY_FORKER
#undef PUD_QUERY_FORKER_TEMPLATE

#endif
