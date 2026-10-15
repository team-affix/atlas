#ifndef PUD_QUERY_HEAD_HPP
#define PUD_QUERY_HEAD_HPP

#include <optional>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_forced_unfold_reason.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_query_resume_context.hpp"
#include "debug_assert.hpp"

template<
    typename ReplayDescent,
    typename ITryAddCandidateSpecializationHead,
    typename ITryForkCandidateSpecializationHead>
struct pud_query_head {
    pud_query_head(
        ITryAddCandidateSpecializationHead& try_add_candidate_specialization_head,
        ITryForkCandidateSpecializationHead& try_fork_candidate_specialization_head,
        const std::vector<ReplayDescent>& root_descents);
    pud_query_head(
        const pud_query_head& other,
        const std::vector<ReplayDescent>& root_descents);
    pud_query_resume_context resume();
    std::optional<pud_forced_unfold_reason> candidate_refuted(pud_mhcs_head_id candidate_id);
private:
    void link(pud_node_id root, pud_mhcs_head_id candidate_id);
    std::optional<pud_forced_unfold_reason> forced_unfold_reason() const;

    ITryAddCandidateSpecializationHead& try_add_candidate_specialization_head_;
    ITryForkCandidateSpecializationHead& try_fork_candidate_specialization_head_;

    std::unordered_map<pud_node_id, pud_mhcs_head_id> root_to_candidate_;
    std::unordered_map<pud_mhcs_head_id, pud_node_id> candidate_to_root_;
};

#define PUD_QUERY_HEAD_TEMPLATE                                                            \
    template<typename ReplayDescent, typename ITryAddCandidateSpecializationHead,          \
             typename ITryForkCandidateSpecializationHead>
#define PUD_QUERY_HEAD                                                                     \
    pud_query_head<ReplayDescent, ITryAddCandidateSpecializationHead,                      \
        ITryForkCandidateSpecializationHead>

PUD_QUERY_HEAD_TEMPLATE
PUD_QUERY_HEAD::pud_query_head(
    ITryAddCandidateSpecializationHead& try_add_candidate_specialization_head,
    ITryForkCandidateSpecializationHead& try_fork_candidate_specialization_head,
    const std::vector<ReplayDescent>& root_descents)
    : try_add_candidate_specialization_head_(try_add_candidate_specialization_head)
    , try_fork_candidate_specialization_head_(try_fork_candidate_specialization_head) {
    for (const ReplayDescent& root_descent : root_descents) {
        std::optional<pud_mhcs_head_id> candidate_id =
            try_add_candidate_specialization_head_.try_add_head(root_descent);
        if (!candidate_id.has_value())
            continue;
        link(root_descent.node, candidate_id.value());
    }
}

PUD_QUERY_HEAD_TEMPLATE
PUD_QUERY_HEAD::pud_query_head(
    const pud_query_head& other,
    const std::vector<ReplayDescent>& root_descents)
    : try_add_candidate_specialization_head_(other.try_add_candidate_specialization_head_)
    , try_fork_candidate_specialization_head_(other.try_fork_candidate_specialization_head_) {
    for (const ReplayDescent& root_descent : root_descents) {
        auto other_candidate_it = other.root_to_candidate_.find(root_descent.node);
        if (other_candidate_it == other.root_to_candidate_.end())
            continue;
        std::optional<pud_mhcs_head_id> candidate_id =
            try_fork_candidate_specialization_head_.try_fork_head(other_candidate_it->second, root_descent);
        if (!candidate_id.has_value())
            continue;
        link(root_descent.node, candidate_id.value());
    }
}

PUD_QUERY_HEAD_TEMPLATE
pud_query_resume_context PUD_QUERY_HEAD::resume() {
    pud_query_resume_context context;
    context.candidate_ids.reserve(candidate_to_root_.size());
    for (const auto& [candidate_id, root] : candidate_to_root_)
        context.candidate_ids.push_back(candidate_id);
    context.forced_unfold_reason = forced_unfold_reason();
    return context;
}

PUD_QUERY_HEAD_TEMPLATE
std::optional<pud_forced_unfold_reason> PUD_QUERY_HEAD::candidate_refuted(
    pud_mhcs_head_id candidate_id) {
    auto extracted = candidate_to_root_.extract(candidate_id);
    DEBUG_ASSERT(!extracted.empty());
    root_to_candidate_.erase(extracted.mapped());
    return forced_unfold_reason();
}

PUD_QUERY_HEAD_TEMPLATE
void PUD_QUERY_HEAD::link(pud_node_id root, pud_mhcs_head_id candidate_id) {
    const bool root_inserted = root_to_candidate_.emplace(root, candidate_id).second;
    DEBUG_ASSERT(root_inserted);
    const bool candidate_inserted = candidate_to_root_.emplace(candidate_id, root).second;
    DEBUG_ASSERT(candidate_inserted);
}

PUD_QUERY_HEAD_TEMPLATE
std::optional<pud_forced_unfold_reason> PUD_QUERY_HEAD::forced_unfold_reason() const {
    if (candidate_to_root_.empty())
        return pud_forced_unfold_reason::refuted;
    if (candidate_to_root_.size() == 1)
        return pud_forced_unfold_reason::unit;
    return std::nullopt;
}

#undef PUD_QUERY_HEAD
#undef PUD_QUERY_HEAD_TEMPLATE

#endif
