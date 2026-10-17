#ifndef PUD_WITNESS_SEARCH_HEAD_FORKER_HPP
#define PUD_WITNESS_SEARCH_HEAD_FORKER_HPP

#include "infrastructure/pud_witness_search_head.hpp"

template<
    typename Descent,
    typename NodeIterator,
    typename ICheckNodeLeaf,
    typename ICheckNodeRefuted,
    typename IGetNodeChildren>
struct pud_witness_search_head_forker {
    using head_type = pud_witness_search_head<
        Descent,
        NodeIterator,
        ICheckNodeLeaf,
        ICheckNodeRefuted,
        IGetNodeChildren>;

    head_type fork(const head_type& other, Descent search_root_descent) const;
};

template<typename D, typename NI, typename ICNL, typename ICNR, typename IGNC>
typename pud_witness_search_head_forker<D, NI, ICNL, ICNR, IGNC>::head_type
pud_witness_search_head_forker<D, NI, ICNL, ICNR, IGNC>::fork(const head_type& other, D search_root_descent) const {
    return head_type{other, search_root_descent};
}

#endif
