#ifndef PUD_LODESTAR_UNIT_PROPAGATOR_HPP
#define PUD_LODESTAR_UNIT_PROPAGATOR_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_lodestar_unfold_outcome.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_unfold.hpp"

// Drains the queue of queries that went unit and unfolds the first held leaf that can be unfolded. A
// query held by more ready leaves goes back to the front of the queue for the next call.
template<
    typename IUnitQueries,
    typename IFindQueryHolder,
    typename IGetQueryHolders,
    typename IGetQueryCandidates,
    typename IGetLiveNodes,
    typename IIsNodeRefuted,
    typename IAllowUnfold,
    typename IUnfoldLeaf>
struct pud_lodestar_unit_propagator {
    pud_lodestar_unit_propagator(
        IUnitQueries& unit_queries,
        IFindQueryHolder& find_query_holder,
        IGetQueryHolders& get_query_holders,
        IGetQueryCandidates& get_query_candidates,
        IGetLiveNodes& get_live_nodes,
        IIsNodeRefuted& is_node_refuted,
        IAllowUnfold& allow_unfold,
        IUnfoldLeaf& unfold_leaf);
    pud_lodestar_unfold_outcome propagate();
private:
    IUnitQueries& unit_queries_;
    IFindQueryHolder& find_query_holder_;
    IGetQueryHolders& get_query_holders_;
    IGetQueryCandidates& get_query_candidates_;
    IGetLiveNodes& get_live_nodes_;
    IIsNodeRefuted& is_node_refuted_;
    IAllowUnfold& allow_unfold_;
    IUnfoldLeaf& unfold_leaf_;
};

#define PUD_LODESTAR_UNIT_PROPAGATOR_TEMPLATE                                                       \
    template<typename IUnitQueries, typename IFindQueryHolder, typename IGetQueryHolders,          \
             typename IGetQueryCandidates, typename IGetLiveNodes, typename IIsNodeRefuted,          \
             typename IAllowUnfold, typename IUnfoldLeaf>
#define PUD_LODESTAR_UNIT_PROPAGATOR                                                                \
    pud_lodestar_unit_propagator<IUnitQueries, IFindQueryHolder, IGetQueryHolders, IGetQueryCandidates, \
        IGetLiveNodes, IIsNodeRefuted, IAllowUnfold, IUnfoldLeaf>

PUD_LODESTAR_UNIT_PROPAGATOR_TEMPLATE
PUD_LODESTAR_UNIT_PROPAGATOR::pud_lodestar_unit_propagator(
    IUnitQueries& unit_queries,
    IFindQueryHolder& find_query_holder,
    IGetQueryHolders& get_query_holders,
    IGetQueryCandidates& get_query_candidates,
    IGetLiveNodes& get_live_nodes,
    IIsNodeRefuted& is_node_refuted,
    IAllowUnfold& allow_unfold,
    IUnfoldLeaf& unfold_leaf)
    : unit_queries_(unit_queries)
    , find_query_holder_(find_query_holder)
    , get_query_holders_(get_query_holders)
    , get_query_candidates_(get_query_candidates)
    , get_live_nodes_(get_live_nodes)
    , is_node_refuted_(is_node_refuted)
    , allow_unfold_(allow_unfold)
    , unfold_leaf_(unfold_leaf) {}

PUD_LODESTAR_UNIT_PROPAGATOR_TEMPLATE
pud_lodestar_unfold_outcome PUD_LODESTAR_UNIT_PROPAGATOR::propagate() {
    while (!unit_queries_.empty()) {
        const pud_mhq_head_id head = unit_queries_.front();
        unit_queries_.pop_front();
        const std::optional<pud_unfold> held = find_query_holder_.find_holder(head);
        if (!held.has_value())
            continue;
        if (get_query_candidates_.candidates(head).size() != 1)
            continue;
        std::vector<pud_global_node_id> ready;
        for (const pud_global_node_id leaf : get_query_holders_.get_holders(head)) {
            if (!get_live_nodes_.live().contains(leaf))
                continue;
            if (is_node_refuted_.is_refuted(leaf))
                continue;
            if (!allow_unfold_.allow(leaf))
                continue;
            ready.push_back(leaf);
        }
        if (ready.empty())
            continue;
        const pud_lodestar_unfold_outcome outcome = unfold_leaf_.unfold(ready.front(), held->goal);
        if (ready.size() > 1)
            unit_queries_.push_front(head);
        return outcome;
    }
    return pud_lodestar_unfold_outcome::none;
}

#undef PUD_LODESTAR_UNIT_PROPAGATOR
#undef PUD_LODESTAR_UNIT_PROPAGATOR_TEMPLATE

#endif
