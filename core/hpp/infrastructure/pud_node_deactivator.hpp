#ifndef PUD_NODE_DEACTIVATOR_HPP
#define PUD_NODE_DEACTIVATOR_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_unfold.hpp"
#include "infrastructure/coroutine.hpp"

template<
    typename IEraseNodeQueries,
    typename IEraseNodeWatch,
    typename IEraseNodeBindings,
    typename IEraseNodeLvc,
    typename IInvalidateQueryLeaf,
    typename IMarkQueryExhausted,
    typename IFindQueryLeaves>
struct pud_node_deactivator {
    pud_node_deactivator(
        IEraseNodeQueries& erase_node_queries,
        IEraseNodeWatch& erase_node_watch,
        IEraseNodeBindings& erase_node_bindings,
        IEraseNodeLvc& erase_node_lvc,
        IInvalidateQueryLeaf& invalidate_query_leaf,
        IMarkQueryExhausted& mark_query_exhausted,
        IFindQueryLeaves& find_query_leaves);
    std::vector<pud_global_node_id> deactivate(pud_global_node_id node);
private:
    IEraseNodeQueries& erase_node_queries_;
    IEraseNodeWatch& erase_node_watch_;
    IEraseNodeBindings& erase_node_bindings_;
    IEraseNodeLvc& erase_node_lvc_;
    IInvalidateQueryLeaf& invalidate_query_leaf_;
    IMarkQueryExhausted& mark_query_exhausted_;
    IFindQueryLeaves& find_query_leaves_;
};

#define PUD_NODE_DEACTIVATOR_TEMPLATE                                                                   \
    template<typename IEraseNodeQueries, typename IEraseNodeWatch, typename IEraseNodeBindings,         \
             typename IEraseNodeLvc, typename IInvalidateQueryLeaf, typename IMarkQueryExhausted,                \
             typename IFindQueryLeaves>
#define PUD_NODE_DEACTIVATOR                                                                            \
    pud_node_deactivator<IEraseNodeQueries, IEraseNodeWatch, IEraseNodeBindings, IEraseNodeLvc,           \
        IInvalidateQueryLeaf, IMarkQueryExhausted, IFindQueryLeaves>

PUD_NODE_DEACTIVATOR_TEMPLATE
PUD_NODE_DEACTIVATOR::pud_node_deactivator(
    IEraseNodeQueries& erase_node_queries,
    IEraseNodeWatch& erase_node_watch,
    IEraseNodeBindings& erase_node_bindings,
    IEraseNodeLvc& erase_node_lvc,
    IInvalidateQueryLeaf& invalidate_query_leaf,
    IMarkQueryExhausted& mark_query_exhausted,
    IFindQueryLeaves& find_query_leaves)
    : erase_node_queries_(erase_node_queries)
    , erase_node_watch_(erase_node_watch)
    , erase_node_bindings_(erase_node_bindings)
    , erase_node_lvc_(erase_node_lvc)
    , invalidate_query_leaf_(invalidate_query_leaf)
    , mark_query_exhausted_(mark_query_exhausted)
    , find_query_leaves_(find_query_leaves) {}

// Erasing the node's queries drops its share of each head owner; a head no other live node shares is removed.
// A query the node's witnesses leave without a candidate is marked exhausted, for the nodes yet to be activated
// under it, and strands every live leaf still holding it.
PUD_NODE_DEACTIVATOR_TEMPLATE
std::vector<pud_global_node_id> PUD_NODE_DEACTIVATOR::deactivate(pud_global_node_id node) {
    erase_node_queries_.erase(node);
    erase_node_watch_.erase(node);
    erase_node_bindings_.erase(node);
    erase_node_lvc_.erase(node);
    std::vector<pud_global_node_id> stranded;
    for (pud_mhq_head_id exhausted : invalidate_query_leaf_.invalidate_leaf(node)) {
        mark_query_exhausted_.mark_exhausted(exhausted);
        coroutine<pud_unfold, void> leaves = find_query_leaves_.find_leaves(exhausted);
        for (std::optional<pud_unfold> leaf = leaves.next(); leaf.has_value(); leaf = leaves.next())
            stranded.push_back(leaf->leaf);
    }
    return stranded;
}

#undef PUD_NODE_DEACTIVATOR
#undef PUD_NODE_DEACTIVATOR_TEMPLATE

#endif
