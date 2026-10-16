#ifndef PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP
#define PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP

#include <utility>
#include "infrastructure/pud_witness_search_head.hpp"

template<
    typename Descent,
    typename NodeIterator,
    typename ICheckNodeLeaf,
    typename ICheckNodeRefuted,
    typename IGetNodeChildren,
    typename IGetCallSiteIdx>
struct pud_witness_search_head_factory {
    using head_type = pud_witness_search_head<
        Descent,
        NodeIterator,
        ICheckNodeLeaf,
        ICheckNodeRefuted,
        IGetNodeChildren,
        IGetCallSiteIdx>;

    pud_witness_search_head_factory(
        ICheckNodeLeaf& check_node_leaf,
        ICheckNodeRefuted& check_node_refuted,
        IGetNodeChildren& get_node_children,
        IGetCallSiteIdx& get_call_site_idx);
    head_type make(Descent search_root_descent) const;
private:
    ICheckNodeLeaf& check_node_leaf_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetNodeChildren& get_node_children_;
    IGetCallSiteIdx& get_call_site_idx_;
};

template<
    typename D,
    typename NI,
    typename ICNL,
    typename ICNR,
    typename IGNC,
    typename IGCSI>
pud_witness_search_head_factory<D, NI, ICNL, ICNR, IGNC, IGCSI>::pud_witness_search_head_factory(
    ICNL& check_node_leaf,
    ICNR& check_node_refuted,
    IGNC& get_node_children,
    IGCSI& get_call_site_idx)
    : check_node_leaf_(check_node_leaf)
    , check_node_refuted_(check_node_refuted)
    , get_node_children_(get_node_children)
    , get_call_site_idx_(get_call_site_idx) {}

template<
    typename D,
    typename NI,
    typename ICNL,
    typename ICNR,
    typename IGNC,
    typename IGCSI>
typename pud_witness_search_head_factory<D, NI, ICNL, ICNR, IGNC, IGCSI>::head_type
pud_witness_search_head_factory<D, NI, ICNL, ICNR, IGNC, IGCSI>::make(D search_root_descent) const {
    return head_type{
        check_node_leaf_,
        check_node_refuted_,
        get_node_children_,
        get_call_site_idx_,
        std::move(search_root_descent)};
}

#endif
