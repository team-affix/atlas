#ifndef PUD_LEAF_INVALIDATOR_HPP
#define PUD_LEAF_INVALIDATOR_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Tells every multi-headed layer that some leaves stopped being live leaves (refuted or
// expanded) and returns the queries that became forced. Every witness search is moved before
// any candidate learns of the witnesses it lost, so no candidate reaches for a witness that is
// gone but not yet reported.
template<
    typename IInvalidateLeaf0,
    typename IInvalidateLeaf1,
    typename IWitnessesRefuted,
    typename ICandidateSpecializationRefuted>
struct pud_leaf_invalidator {
    pud_leaf_invalidator(
        IInvalidateLeaf0& invalidate_leaf_0,
        IInvalidateLeaf1& invalidate_leaf_1,
        IWitnessesRefuted& witnesses_refuted,
        ICandidateSpecializationRefuted& candidate_specialization_refuted);
    std::vector<pud_forced_query_propagation> invalidate(const std::vector<pud_node_id>& leaves);
private:
    IInvalidateLeaf0& invalidate_leaf_0_;
    IInvalidateLeaf1& invalidate_leaf_1_;
    IWitnessesRefuted& witnesses_refuted_;
    ICandidateSpecializationRefuted& candidate_specialization_refuted_;
};

#define PUD_LEAF_INVALIDATOR_TEMPLATE                                                      \
    template<typename IInvalidateLeaf0, typename IInvalidateLeaf1, typename IWitnessesRefuted, \
             typename ICandidateSpecializationRefuted>
#define PUD_LEAF_INVALIDATOR                                                               \
    pud_leaf_invalidator<IInvalidateLeaf0, IInvalidateLeaf1, IWitnessesRefuted,            \
        ICandidateSpecializationRefuted>

PUD_LEAF_INVALIDATOR_TEMPLATE
PUD_LEAF_INVALIDATOR::pud_leaf_invalidator(
    IInvalidateLeaf0& invalidate_leaf_0,
    IInvalidateLeaf1& invalidate_leaf_1,
    IWitnessesRefuted& witnesses_refuted,
    ICandidateSpecializationRefuted& candidate_specialization_refuted)
    : invalidate_leaf_0_(invalidate_leaf_0)
    , invalidate_leaf_1_(invalidate_leaf_1)
    , witnesses_refuted_(witnesses_refuted)
    , candidate_specialization_refuted_(candidate_specialization_refuted) {}

PUD_LEAF_INVALIDATOR_TEMPLATE
std::vector<pud_forced_query_propagation> PUD_LEAF_INVALIDATOR::invalidate(const std::vector<pud_node_id>& leaves) {
    std::vector<pud_mhws_head_id> exhausted_witnesses;
    for (pud_node_id leaf : leaves) {
        const std::vector<pud_mhws_head_id> leaf_exhausted = invalidate_leaf_0_.invalidate_leaf(leaf);
        exhausted_witnesses.insert(exhausted_witnesses.end(), leaf_exhausted.begin(), leaf_exhausted.end());
    }

    std::vector<pud_mhcs_head_id> refuted_candidates;
    for (pud_node_id leaf : leaves) {
        const std::vector<pud_mhcs_head_id> leaf_refuted = invalidate_leaf_1_.invalidate_leaf(leaf);
        refuted_candidates.insert(refuted_candidates.end(), leaf_refuted.begin(), leaf_refuted.end());
    }
    const std::vector<pud_mhcs_head_id> unjustified = witnesses_refuted_.witnesses_refuted(exhausted_witnesses);
    refuted_candidates.insert(refuted_candidates.end(), unjustified.begin(), unjustified.end());

    std::vector<pud_forced_query_propagation> forced_queries;
    for (pud_mhcs_head_id candidate : refuted_candidates) {
        std::optional<pud_forced_query_propagation> forced =
            candidate_specialization_refuted_.candidate_specialization_refuted(candidate);
        if (!forced.has_value())
            continue;
        forced_queries.push_back(forced.value());
    }
    return forced_queries;
}

#undef PUD_LEAF_INVALIDATOR
#undef PUD_LEAF_INVALIDATOR_TEMPLATE

#endif
