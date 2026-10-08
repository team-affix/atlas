#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP

#include "infrastructure/pud_candidate_specialization_head.hpp"

template<
    typename Descent,
    typename ChildIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename IForkWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IPropagateDescent>
struct pud_candidate_specialization_head_forker {
    using head_type = pud_candidate_specialization_head<
        Descent,
        ChildIterator,
        ITryAddHead,
        IAdvanceWitnessSearchHead,
        IForkWitnessSearchHead,
        ICheckNodeLeaf,
        IGetChildren,
        IPropagateDescent>;

    head_type fork(const head_type& other, Descent search_root_descent) const;
};

template<
    typename D,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IDESC>
typename pud_candidate_specialization_head_forker<D, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::head_type
pud_candidate_specialization_head_forker<D, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IDESC>::fork(const head_type& other, D search_root_descent) const {
    return head_type{other, search_root_descent};
}

#endif
