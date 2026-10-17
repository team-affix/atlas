#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP

#include "infrastructure/pud_candidate_specialization_head.hpp"

template<
    typename Descent,
    typename NodeIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename ITryForkWitnessSearchHead,
    typename IRemoveWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename ICheckNodeRefuted,
    typename IGetChildren>
struct pud_candidate_specialization_head_forker {
    using head_type = pud_candidate_specialization_head<
        Descent,
        NodeIterator,
        ITryAddHead,
        IAdvanceWitnessSearchHead,
        ITryForkWitnessSearchHead,
        IRemoveWitnessSearchHead,
        ICheckNodeLeaf,
        ICheckNodeRefuted,
        IGetChildren>;

    head_type fork(const head_type& other, Descent search_root_descent) const;
};

template<typename D, typename NI, typename ITAH, typename IAWSH, typename ITFWSH, typename IRWSH,
         typename ICNL, typename ICNR, typename IGC>
typename pud_candidate_specialization_head_forker<D, NI, ITAH, IAWSH, ITFWSH, IRWSH, ICNL, ICNR, IGC>::head_type
pud_candidate_specialization_head_forker<D, NI, ITAH, IAWSH, ITFWSH, IRWSH, ICNL, ICNR, IGC>::fork(
    const head_type& other, D search_root_descent) const {
    return head_type{other, search_root_descent};
}

#endif
