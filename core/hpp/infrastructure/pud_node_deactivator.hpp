#ifndef PUD_NODE_DEACTIVATOR_HPP
#define PUD_NODE_DEACTIVATOR_HPP

#include <set>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_global_node_id.hpp"

template<
    typename IGetNodeQueries,
    typename IEraseNodeQueries,
    typename IEraseNodeWatch,
    typename IEraseNodeBindings,
    typename IEraseNodeLvc,
    typename IReleaseQuery,
    typename IInvalidateQueryLeaf,
    typename IGetQueryHolders>
struct pud_node_deactivator {
    pud_node_deactivator(
        IGetNodeQueries& get_node_queries,
        IEraseNodeQueries& erase_node_queries,
        IEraseNodeWatch& erase_node_watch,
        IEraseNodeBindings& erase_node_bindings,
        IEraseNodeLvc& erase_node_lvc,
        IReleaseQuery& release_query,
        IInvalidateQueryLeaf& invalidate_query_leaf,
        IGetQueryHolders& get_query_holders);
    std::vector<pud_global_node_id> deactivate(pud_global_node_id node);
private:
    IGetNodeQueries& get_node_queries_;
    IEraseNodeQueries& erase_node_queries_;
    IEraseNodeWatch& erase_node_watch_;
    IEraseNodeBindings& erase_node_bindings_;
    IEraseNodeLvc& erase_node_lvc_;
    IReleaseQuery& release_query_;
    IInvalidateQueryLeaf& invalidate_query_leaf_;
    IGetQueryHolders& get_query_holders_;
};

#define PUD_NODE_DEACTIVATOR_TEMPLATE                                                      \
    template<typename IGetNodeQueries, typename IEraseNodeQueries, typename IEraseNodeWatch,         \
             typename IEraseNodeBindings, typename IEraseNodeLvc, typename IReleaseQuery,           \
             typename IInvalidateQueryLeaf, typename IGetQueryHolders>
#define PUD_NODE_DEACTIVATOR                                                               \
    pud_node_deactivator<IGetNodeQueries, IEraseNodeQueries, IEraseNodeWatch, IEraseNodeBindings,    \
        IEraseNodeLvc, IReleaseQuery,                                                  \
        IInvalidateQueryLeaf, IGetQueryHolders>

PUD_NODE_DEACTIVATOR_TEMPLATE
PUD_NODE_DEACTIVATOR::pud_node_deactivator(
    IGetNodeQueries& get_node_queries,
    IEraseNodeQueries& erase_node_queries,
    IEraseNodeWatch& erase_node_watch,
    IEraseNodeBindings& erase_node_bindings,
    IEraseNodeLvc& erase_node_lvc,
    IReleaseQuery& release_query,
    IInvalidateQueryLeaf& invalidate_query_leaf,
    IGetQueryHolders& get_query_holders)
    : get_node_queries_(get_node_queries)
    , erase_node_queries_(erase_node_queries)
    , erase_node_watch_(erase_node_watch)
    , erase_node_bindings_(erase_node_bindings)
    , erase_node_lvc_(erase_node_lvc)
    , release_query_(release_query)
    , invalidate_query_leaf_(invalidate_query_leaf)
    , get_query_holders_(get_query_holders) {}

PUD_NODE_DEACTIVATOR_TEMPLATE
std::vector<pud_global_node_id> PUD_NODE_DEACTIVATOR::deactivate(pud_global_node_id node) {
    for (const auto& [goal, owner] : get_node_queries_.get(node))
        release_query_.release(owner->head_id(), node);
    // Erasing the node's queries drops its share of each head owner; a head no other live node shares is removed.
    erase_node_queries_.erase(node);
    erase_node_watch_.erase(node);
    erase_node_bindings_.erase(node);
    erase_node_lvc_.erase(node);
    std::vector<pud_global_node_id> stranded;
    for (pud_mhq_head_id exhausted : invalidate_query_leaf_.invalidate_leaf(node)) {
        const std::set<pud_global_node_id>& holders = get_query_holders_.get_holders(exhausted);
        stranded.insert(stranded.end(), holders.begin(), holders.end());
    }
    return stranded;
}

#undef PUD_NODE_DEACTIVATOR
#undef PUD_NODE_DEACTIVATOR_TEMPLATE

#endif
