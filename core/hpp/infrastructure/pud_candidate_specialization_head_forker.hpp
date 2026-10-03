#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP

#include "infrastructure/pud_candidate_specialization_head.hpp"

template<
    typename QueryHandle,
    typename ChildIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename IForkWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IPropagateQueryHandle>
struct pud_candidate_specialization_head_forker {
    using head_type = pud_candidate_specialization_head<
        QueryHandle,
        ChildIterator,
        ITryAddHead,
        IAdvanceWitnessSearchHead,
        IForkWitnessSearchHead,
        ICheckNodeLeaf,
        IGetChildren,
        IPropagateQueryHandle>;

    head_type fork(const head_type& other, QueryHandle search_root_handle) const;
};

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
typename pud_candidate_specialization_head_forker<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::head_type
pud_candidate_specialization_head_forker<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::fork(const head_type& other, QH search_root_handle) const {
    return head_type{other, search_root_handle};
}

#endif
