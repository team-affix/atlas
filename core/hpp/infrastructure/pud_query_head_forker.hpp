#ifndef PUD_QUERY_HEAD_FORKER_HPP
#define PUD_QUERY_HEAD_FORKER_HPP

#include <vector>
#include "infrastructure/pud_query_head.hpp"

template<
    typename ReplayDescent,
    typename ITryAddCandidateSpecializationHead,
    typename ITryForkCandidateSpecializationHead>
struct pud_query_head_forker {
    using head_type = pud_query_head<
        ReplayDescent,
        ITryAddCandidateSpecializationHead,
        ITryForkCandidateSpecializationHead>;

    head_type fork(const head_type& other, const std::vector<ReplayDescent>& root_descents) const;
};

template<typename RD, typename ITACSH, typename ITFCSH>
typename pud_query_head_forker<RD, ITACSH, ITFCSH>::head_type
pud_query_head_forker<RD, ITACSH, ITFCSH>::fork(
    const head_type& other,
    const std::vector<RD>& root_descents) const {
    return head_type{other, root_descents};
}

#endif
