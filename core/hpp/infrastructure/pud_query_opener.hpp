#ifndef PUD_QUERY_OPENER_HPP
#define PUD_QUERY_OPENER_HPP

#include <optional>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Opens a query for one of a leaf's goals, in the leaf's environment, over the axioms its group sees.
// None when no axiom yields a candidate.
template<typename IOpenQuery, typename IFindNodeGoal, typename IGetNodeLvc, typename IGetNodeAxiomGroup>
struct pud_query_opener {
    pud_query_opener(
        IOpenQuery& open_query,
        IFindNodeGoal& find_node_goal,
        IGetNodeLvc& get_node_lvc,
        IGetNodeAxiomGroup& get_node_axiom_group);
    std::optional<pud_mhq_head_id> open(pud_node_id leaf, const pud_ubs_state& bindings, pud_body_goal_id goal);
private:
    IOpenQuery& open_query_;
    IFindNodeGoal& find_node_goal_;
    IGetNodeLvc& get_node_lvc_;
    IGetNodeAxiomGroup& get_node_axiom_group_;
};

#define PUD_QUERY_OPENER_TEMPLATE                                                          \
    template<typename IOpenQuery, typename IFindNodeGoal, typename IGetNodeLvc, typename IGetNodeAxiomGroup>
#define PUD_QUERY_OPENER pud_query_opener<IOpenQuery, IFindNodeGoal, IGetNodeLvc, IGetNodeAxiomGroup>

PUD_QUERY_OPENER_TEMPLATE
PUD_QUERY_OPENER::pud_query_opener(
    IOpenQuery& open_query,
    IFindNodeGoal& find_node_goal,
    IGetNodeLvc& get_node_lvc,
    IGetNodeAxiomGroup& get_node_axiom_group)
    : open_query_(open_query)
    , find_node_goal_(find_node_goal)
    , get_node_lvc_(get_node_lvc)
    , get_node_axiom_group_(get_node_axiom_group) {}

PUD_QUERY_OPENER_TEMPLATE
std::optional<pud_mhq_head_id> PUD_QUERY_OPENER::open(
    pud_node_id leaf,
    const pud_ubs_state& bindings,
    pud_body_goal_id goal) {
    return open_query_.try_open(
        get_node_axiom_group_.get(leaf), bindings, find_node_goal_.find(leaf, goal), get_node_lvc_.get(leaf));
}

#undef PUD_QUERY_OPENER
#undef PUD_QUERY_OPENER_TEMPLATE

#endif
