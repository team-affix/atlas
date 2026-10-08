#ifndef PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP
#define PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP

#include <utility>
#include "infrastructure/pud_witness_search_head.hpp"

template<
    typename QueryHandle,
    typename NodeIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IDescendQueryNodeHandle,
    typename IGetCallSiteIdx>
struct pud_witness_search_head_factory {
    using head_type = pud_witness_search_head<
        QueryHandle,
        NodeIterator,
        ICheckNodeLeaf,
        IGetNodeChildren,
        IDescendQueryNodeHandle,
        IGetCallSiteIdx>;

    pud_witness_search_head_factory(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IDescendQueryNodeHandle& descend_query_node_handle,
        IGetCallSiteIdx& get_call_site_idx);
    head_type make(QueryHandle search_root_handle) const;
private:
    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IDescendQueryNodeHandle& descend_query_node_handle_;
    IGetCallSiteIdx& get_call_site_idx_;
};

template<
    typename QH,
    typename NI,
    typename ICNL,
    typename IGNC,
    typename IDQN,
    typename IGCSI>
pud_witness_search_head_factory<QH, NI, ICNL, IGNC, IDQN, IGCSI>::pud_witness_search_head_factory(
    ICNL& check_node_leaf,
    IGNC& get_node_children,
    IDQN& descend_query_node_handle,
    IGCSI& get_call_site_idx)
    : check_node_leaf_(check_node_leaf)
    , get_node_children_(get_node_children)
    , descend_query_node_handle_(descend_query_node_handle)
    , get_call_site_idx_(get_call_site_idx) {}

template<
    typename QH,
    typename NI,
    typename ICNL,
    typename IGNC,
    typename IDQN,
    typename IGCSI>
typename pud_witness_search_head_factory<QH, NI, ICNL, IGNC, IDQN, IGCSI>::head_type
pud_witness_search_head_factory<QH, NI, ICNL, IGNC, IDQN, IGCSI>::make(QH search_root_handle) const {
    return head_type{
        check_node_leaf_,
        get_node_children_,
        descend_query_node_handle_,
        get_call_site_idx_,
        std::move(search_root_handle)};
}

#endif
