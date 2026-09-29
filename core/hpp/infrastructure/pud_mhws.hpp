#ifndef PUD_MHWS_HPP
#define PUD_MHWS_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/pud_witness_search_head.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

template<
    typename QueryPosition,
    typename ChildIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IPropagateQueryNodeHandle,
    typename IGetCallSiteIdx>
struct pud_mhws {
    pud_mhws(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IPropagateQueryNodeHandle& propagate_query_node_handle,
        IGetCallSiteIdx& get_call_site_idx);
    std::optional<pud_mhws_head_id> try_add_head(QueryPosition search_root_position);
    void remove_head(pud_mhws_head_id head_id);
    void invalidate_leaf(const pud_node* node);
private:
    using head_type = pud_witness_search_head<
        QueryPosition,
        ChildIterator,
        ICheckNodeLeaf,
        IGetNodeChildren,
        IPropagateQueryNodeHandle,
        IGetCallSiteIdx
    >;

    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IPropagateQueryNodeHandle& propagate_query_node_handle_;
    IGetCallSiteIdx& get_call_site_idx_;

    pud_mhws_head_id next_head_id_;
    std::unordered_map<pud_mhws_head_id, head_type> heads_;
    std::unordered_map<pud_mhws_head_id, const pud_node*> head_to_witness_;
    std::unordered_map<const pud_node*, std::unordered_set<pud_mhws_head_id>> witness_to_heads_;
};

#endif
