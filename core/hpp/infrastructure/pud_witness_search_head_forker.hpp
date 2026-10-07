#ifndef PUD_WITNESS_SEARCH_HEAD_FORKER_HPP
#define PUD_WITNESS_SEARCH_HEAD_FORKER_HPP

#include "infrastructure/pud_witness_search_head.hpp"

template<
    typename QueryHandle,
    typename ChildIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IPropagateQueryNodeHandle,
    typename IGetCallSiteIdx>
struct pud_witness_search_head_forker {
    using head_type = pud_witness_search_head<
        QueryHandle,
        ChildIterator,
        ICheckNodeLeaf,
        IGetNodeChildren,
        IPropagateQueryNodeHandle,
        IGetCallSiteIdx>;

    head_type fork(const head_type& other, QueryHandle search_root_handle) const;
};

template<
    typename QH,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IPQN,
    typename IGCSI>
typename pud_witness_search_head_forker<QH, CI, ICNL, IGNC, IPQN, IGCSI>::head_type
pud_witness_search_head_forker<QH, CI, ICNL, IGNC, IPQN, IGCSI>::fork(const head_type& other, QH search_root_handle) const {
    return head_type{other, search_root_handle};
}

#endif
