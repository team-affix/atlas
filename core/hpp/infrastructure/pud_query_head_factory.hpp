#ifndef PUD_QUERY_HEAD_FACTORY_HPP
#define PUD_QUERY_HEAD_FACTORY_HPP

#include <vector>
#include "infrastructure/pud_query_head.hpp"

template<
    typename ReplayDescent,
    typename ITryAddCandidateSpecializationHead,
    typename ITryForkCandidateSpecializationHead>
struct pud_query_head_factory {
    using head_type = pud_query_head<
        ReplayDescent,
        ITryAddCandidateSpecializationHead,
        ITryForkCandidateSpecializationHead>;

    pud_query_head_factory(
        ITryAddCandidateSpecializationHead& try_add_candidate_specialization_head,
        ITryForkCandidateSpecializationHead& try_fork_candidate_specialization_head);
    head_type make(const std::vector<ReplayDescent>& root_descents) const;
private:
    ITryAddCandidateSpecializationHead& try_add_candidate_specialization_head_;
    ITryForkCandidateSpecializationHead& try_fork_candidate_specialization_head_;
};

template<typename RD, typename ITACSH, typename ITFCSH>
pud_query_head_factory<RD, ITACSH, ITFCSH>::pud_query_head_factory(
    ITACSH& try_add_candidate_specialization_head,
    ITFCSH& try_fork_candidate_specialization_head)
    : try_add_candidate_specialization_head_(try_add_candidate_specialization_head)
    , try_fork_candidate_specialization_head_(try_fork_candidate_specialization_head) {}

template<typename RD, typename ITACSH, typename ITFCSH>
typename pud_query_head_factory<RD, ITACSH, ITFCSH>::head_type
pud_query_head_factory<RD, ITACSH, ITFCSH>::make(const std::vector<RD>& root_descents) const {
    return head_type{
        try_add_candidate_specialization_head_,
        try_fork_candidate_specialization_head_,
        root_descents};
}

#endif
