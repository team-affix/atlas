#ifndef PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP
#define PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP

#include <utility>
#include "infrastructure/pud_witness_search_head.hpp"

template<
    typename Descent,
    typename NodeIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IDescend,
    typename IGetCallSiteIdx>
struct pud_witness_search_head_factory {
    using head_type = pud_witness_search_head<
        Descent,
        NodeIterator,
        ICheckNodeLeaf,
        IGetNodeChildren,
        IDescend,
        IGetCallSiteIdx>;

    pud_witness_search_head_factory(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IDescend& descend,
        IGetCallSiteIdx& get_call_site_idx);
    head_type make(Descent search_root_descent) const;
private:
    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IDescend& descend_;
    IGetCallSiteIdx& get_call_site_idx_;
};

template<
    typename D,
    typename NI,
    typename ICNL,
    typename IGNC,
    typename IDESC,
    typename IGCSI>
pud_witness_search_head_factory<D, NI, ICNL, IGNC, IDESC, IGCSI>::pud_witness_search_head_factory(
    ICNL& check_node_leaf,
    IGNC& get_node_children,
    IDESC& descend,
    IGCSI& get_call_site_idx)
    : check_node_leaf_(check_node_leaf)
    , get_node_children_(get_node_children)
    , descend_(descend)
    , get_call_site_idx_(get_call_site_idx) {}

template<
    typename D,
    typename NI,
    typename ICNL,
    typename IGNC,
    typename IDESC,
    typename IGCSI>
typename pud_witness_search_head_factory<D, NI, ICNL, IGNC, IDESC, IGCSI>::head_type
pud_witness_search_head_factory<D, NI, ICNL, IGNC, IDESC, IGCSI>::make(D search_root_descent) const {
    return head_type{
        check_node_leaf_,
        get_node_children_,
        descend_,
        get_call_site_idx_,
        std::move(search_root_descent)};
}

#endif
