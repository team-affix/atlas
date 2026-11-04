#ifndef PUD_UNIT_PROPAGATOR_HPP
#define PUD_UNIT_PROPAGATOR_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_unfold_outcome.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_unfold.hpp"
#include "infrastructure/coroutine.hpp"

// Drains the queue of queries that went unit and unfolds the first leaf holding one that can be unfolded. The
// leaves are found by walking down from where the query was introduced, only as far as the second ready leaf; a
// query with more ready leaves goes back to the front of the queue for the next call.
template<
    typename IUnitQueries,
    typename IFindQueryLeaves,
    typename IGetQueryCandidates,
    typename IGetLiveNodes,
    typename IIsNodeRefuted,
    typename IAllowUnfold,
    typename IUnfoldLeaf>
struct pud_unit_propagator {
    pud_unit_propagator(
        IUnitQueries& unit_queries,
        IFindQueryLeaves& find_query_leaves,
        IGetQueryCandidates& get_query_candidates,
        IGetLiveNodes& get_live_nodes,
        IIsNodeRefuted& is_node_refuted,
        IAllowUnfold& allow_unfold,
        IUnfoldLeaf& unfold_leaf);
    pud_unfold_outcome propagate();
private:
    IUnitQueries& unit_queries_;
    IFindQueryLeaves& find_query_leaves_;
    IGetQueryCandidates& get_query_candidates_;
    IGetLiveNodes& get_live_nodes_;
    IIsNodeRefuted& is_node_refuted_;
    IAllowUnfold& allow_unfold_;
    IUnfoldLeaf& unfold_leaf_;
};

#define PUD_UNIT_PROPAGATOR_TEMPLATE                                                                    \
    template<typename IUnitQueries, typename IFindQueryLeaves, typename IGetQueryCandidates,            \
             typename IGetLiveNodes, typename IIsNodeRefuted, typename IAllowUnfold, typename IUnfoldLeaf>
#define PUD_UNIT_PROPAGATOR                                                                             \
    pud_unit_propagator<IUnitQueries, IFindQueryLeaves, IGetQueryCandidates, IGetLiveNodes, IIsNodeRefuted, \
        IAllowUnfold, IUnfoldLeaf>

PUD_UNIT_PROPAGATOR_TEMPLATE
PUD_UNIT_PROPAGATOR::pud_unit_propagator(
    IUnitQueries& unit_queries,
    IFindQueryLeaves& find_query_leaves,
    IGetQueryCandidates& get_query_candidates,
    IGetLiveNodes& get_live_nodes,
    IIsNodeRefuted& is_node_refuted,
    IAllowUnfold& allow_unfold,
    IUnfoldLeaf& unfold_leaf)
    : unit_queries_(unit_queries)
    , find_query_leaves_(find_query_leaves)
    , get_query_candidates_(get_query_candidates)
    , get_live_nodes_(get_live_nodes)
    , is_node_refuted_(is_node_refuted)
    , allow_unfold_(allow_unfold)
    , unfold_leaf_(unfold_leaf) {}

// A removed query has no introduction left, so its walk finds no leaf and its candidates are never read.
PUD_UNIT_PROPAGATOR_TEMPLATE
pud_unfold_outcome PUD_UNIT_PROPAGATOR::propagate() {
    while (!unit_queries_.empty()) {
        const pud_mhq_head_id head = unit_queries_.front();
        unit_queries_.pop_front();
        coroutine<pud_unfold, void> leaves = find_query_leaves_.find_leaves(head);
        std::optional<pud_unfold> first_ready;
        bool more_ready = false;
        for (std::optional<pud_unfold> held = leaves.next(); held.has_value(); held = leaves.next()) {
            if (!get_live_nodes_.live().contains(held->leaf))
                continue;
            if (is_node_refuted_.is_refuted(held->leaf))
                continue;
            if (!allow_unfold_.allow(held->leaf))
                continue;
            if (first_ready.has_value()) {
                more_ready = true;
                break;
            }
            first_ready = held;
        }
        if (!first_ready.has_value())
            continue;
        if (get_query_candidates_.candidates(head).size() != 1)
            continue;
        const pud_unfold_outcome outcome = unfold_leaf_.unfold(first_ready->leaf, first_ready->goal);
        if (more_ready)
            unit_queries_.push_front(head);
        return outcome;
    }
    return pud_unfold_outcome::none;
}

#undef PUD_UNIT_PROPAGATOR
#undef PUD_UNIT_PROPAGATOR_TEMPLATE

#endif
