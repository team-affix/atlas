#ifndef PUD_UNIT_QUERY_RECORDER_HPP
#define PUD_UNIT_QUERY_RECORDER_HPP

#include <optional>
#include <variant>
#include <vector>
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhq_head_admission.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// Wraps the query layer's admissions and candidate refutations: every query head they report as
// unit is pushed onto the unit queries, and every result is passed through unchanged.
template<
    typename ReplayDescent,
    typename ITryAddQueryHead,
    typename ITryForkQueryHead,
    typename ICandidateSpecializationRefuted,
    typename IPushUnitQuery>
struct pud_unit_query_recorder {
    pud_unit_query_recorder(
        ITryAddQueryHead& try_add_query_head,
        ITryForkQueryHead& try_fork_query_head,
        ICandidateSpecializationRefuted& candidate_specialization_refuted,
        IPushUnitQuery& push_unit_query);
    pud_mhq_head_admission try_add_head(const std::vector<ReplayDescent>& root_descents);
    pud_mhq_head_admission try_fork_head(pud_mhq_head_id head_id, const std::vector<ReplayDescent>& root_descents);
    std::optional<pud_forced_query_propagation> candidate_specialization_refuted(pud_mhcs_head_id candidate_head_id);
private:
    void record(const std::optional<pud_forced_query_propagation>& forced_propagation);

    ITryAddQueryHead& try_add_query_head_;
    ITryForkQueryHead& try_fork_query_head_;
    ICandidateSpecializationRefuted& candidate_specialization_refuted_;
    IPushUnitQuery& push_unit_query_;
};

#define PUD_UNIT_QUERY_RECORDER_TEMPLATE                                                   \
    template<typename ReplayDescent, typename ITryAddQueryHead, typename ITryForkQueryHead, \
             typename ICandidateSpecializationRefuted, typename IPushUnitQuery>
#define PUD_UNIT_QUERY_RECORDER                                                            \
    pud_unit_query_recorder<ReplayDescent, ITryAddQueryHead, ITryForkQueryHead,            \
        ICandidateSpecializationRefuted, IPushUnitQuery>

PUD_UNIT_QUERY_RECORDER_TEMPLATE
PUD_UNIT_QUERY_RECORDER::pud_unit_query_recorder(
    ITryAddQueryHead& try_add_query_head,
    ITryForkQueryHead& try_fork_query_head,
    ICandidateSpecializationRefuted& candidate_specialization_refuted,
    IPushUnitQuery& push_unit_query)
    : try_add_query_head_(try_add_query_head)
    , try_fork_query_head_(try_fork_query_head)
    , candidate_specialization_refuted_(candidate_specialization_refuted)
    , push_unit_query_(push_unit_query) {}

PUD_UNIT_QUERY_RECORDER_TEMPLATE
pud_mhq_head_admission PUD_UNIT_QUERY_RECORDER::try_add_head(const std::vector<ReplayDescent>& root_descents) {
    const pud_mhq_head_admission admission = try_add_query_head_.try_add_head(root_descents);
    record(admission.forced_propagation);
    return admission;
}

PUD_UNIT_QUERY_RECORDER_TEMPLATE
pud_mhq_head_admission PUD_UNIT_QUERY_RECORDER::try_fork_head(
    pud_mhq_head_id head_id,
    const std::vector<ReplayDescent>& root_descents) {
    const pud_mhq_head_admission admission = try_fork_query_head_.try_fork_head(head_id, root_descents);
    record(admission.forced_propagation);
    return admission;
}

PUD_UNIT_QUERY_RECORDER_TEMPLATE
std::optional<pud_forced_query_propagation> PUD_UNIT_QUERY_RECORDER::candidate_specialization_refuted(
    pud_mhcs_head_id candidate_head_id) {
    const std::optional<pud_forced_query_propagation> forced =
        candidate_specialization_refuted_.candidate_specialization_refuted(candidate_head_id);
    record(forced);
    return forced;
}

PUD_UNIT_QUERY_RECORDER_TEMPLATE
void PUD_UNIT_QUERY_RECORDER::record(const std::optional<pud_forced_query_propagation>& forced_propagation) {
    if (!forced_propagation.has_value())
        return;
    const pud_unit_query* unit_query = std::get_if<pud_unit_query>(&forced_propagation.value());
    if (!unit_query)
        return;
    push_unit_query_.push(unit_query->head_id);
}

#undef PUD_UNIT_QUERY_RECORDER
#undef PUD_UNIT_QUERY_RECORDER_TEMPLATE

#endif
