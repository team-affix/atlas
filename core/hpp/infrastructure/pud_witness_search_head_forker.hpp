#ifndef PUD_WITNESS_SEARCH_HEAD_FORKER_HPP
#define PUD_WITNESS_SEARCH_HEAD_FORKER_HPP

#include "infrastructure/pud_witness_search_head.hpp"

template<
    typename Descent,
    typename ChildIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IDescend,
    typename IGetCallSiteIdx>
struct pud_witness_search_head_forker {
    using head_type = pud_witness_search_head<
        Descent,
        ChildIterator,
        ICheckNodeLeaf,
        IGetNodeChildren,
        IDescend,
        IGetCallSiteIdx>;

    head_type fork(const head_type& other, Descent search_root_descent) const;
};

template<
    typename D,
    typename CI,
    typename ICNL,
    typename IGNC,
    typename IDQN,
    typename IGCSI>
typename pud_witness_search_head_forker<D, CI, ICNL, IGNC, IDQN, IGCSI>::head_type
pud_witness_search_head_forker<D, CI, ICNL, IGNC, IDQN, IGCSI>::fork(const head_type& other, D search_root_descent) const {
    return head_type{other, search_root_descent};
}

#endif
