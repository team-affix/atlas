#ifndef PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP
#define PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP

#include <utility>
#include "infrastructure/pud_witness_search_head.hpp"

template<
    typename QueryHandle,
    typename ChildIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IPropagateQueryNodeHandle,
    typename IGetCallSiteIdx>
struct pud_witness_search_head_factory {
    using head_type = pud_witness_search_head<
        QueryHandle,
        ChildIterator,
        ICheckNodeLeaf,
        IGetNodeChildren,
        IPropagateQueryNodeHandle,
        IGetCallSiteIdx>;

    pud_witness_search_head_factory(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IPropagateQueryNodeHandle& propagate_query_node_handle,
        IGetCallSiteIdx& get_call_site_idx);
    head_type make(pud_query_position<QueryHandle> search_root_position) const;
private:
    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IPropagateQueryNodeHandle& propagate_query_node_handle_;
    IGetCallSiteIdx& get_call_site_idx_;
};

template<
    typename QH,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
pud_witness_search_head_factory<QH, CI, ICNL, IGNC, IPQN, IGCSI>::pud_witness_search_head_factory(
    ICNL& check_node_leaf,
    IGNC& get_node_children,
    IPQN& propagate_query_node_handle,
    IGCSI& get_call_site_idx)
    : check_node_leaf_(check_node_leaf)
    , get_node_children_(get_node_children)
    , propagate_query_node_handle_(propagate_query_node_handle)
    , get_call_site_idx_(get_call_site_idx) {}

template<
    typename QH,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
typename pud_witness_search_head_factory<QH, CI, ICNL, IGNC, IPQN, IGCSI>::head_type
pud_witness_search_head_factory<QH, CI, ICNL, IGNC, IPQN, IGCSI>::make(pud_query_position<QH> search_root_position) const {
    return head_type{
        check_node_leaf_,
        get_node_children_,
        propagate_query_node_handle_,
        get_call_site_idx_,
        std::move(search_root_position)};
}

#endif
