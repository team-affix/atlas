#ifndef PUD_QUERY_HEAD_FACTORY_HPP
#define PUD_QUERY_HEAD_FACTORY_HPP

#include <vector>

// Makes query heads of the given type, which must be constructible from the candidate specialization
// capabilities and the root descents of a query.
template<
    typename Head,
    typename ReplayDescent,
    typename ITryAddCandidateSpecializationHead,
    typename ITryForkCandidateSpecializationHead>
struct pud_query_head_factory {
    pud_query_head_factory(
        ITryAddCandidateSpecializationHead& try_add_candidate_specialization_head,
        ITryForkCandidateSpecializationHead& try_fork_candidate_specialization_head);
    Head make(const std::vector<ReplayDescent>& root_descents) const;
private:
    ITryAddCandidateSpecializationHead& try_add_candidate_specialization_head_;
    ITryForkCandidateSpecializationHead& try_fork_candidate_specialization_head_;
};

template<typename H, typename RD, typename ITACSH, typename ITFCSH>
pud_query_head_factory<H, RD, ITACSH, ITFCSH>::pud_query_head_factory(
    ITACSH& try_add_candidate_specialization_head,
    ITFCSH& try_fork_candidate_specialization_head)
    : try_add_candidate_specialization_head_(try_add_candidate_specialization_head)
    , try_fork_candidate_specialization_head_(try_fork_candidate_specialization_head) {}

template<typename H, typename RD, typename ITACSH, typename ITFCSH>
H pud_query_head_factory<H, RD, ITACSH, ITFCSH>::make(const std::vector<RD>& root_descents) const {
    return H{try_add_candidate_specialization_head_, try_fork_candidate_specialization_head_, root_descents};
}

#endif
