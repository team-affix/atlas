#ifndef PUD_MHQ_HPP
#define PUD_MHQ_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_forced_unfold_reason.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhq_head_admission.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_query_resume_context.hpp"
#include "debug_assert.hpp"

template<
    typename ReplayDescent,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
struct pud_mhq {
    pud_mhq(IMakeHead& make_head, IForkHead& fork_head);
    pud_mhq_head_admission try_add_head(const std::vector<ReplayDescent>& root_descents);
    void remove_head(pud_mhq_head_id head_id);
    std::optional<pud_forced_query_propagation> candidate_specialization_refuted(pud_mhcs_head_id candidate_head_id);
    pud_mhq_head_admission try_fork_head(pud_mhq_head_id head_id, const std::vector<ReplayDescent>& root_descents);
private:
    pud_mhq_head_admission try_keep_head(
        pud_mhq_head_id head_id,
        const pud_query_resume_context& context);
    void link(pud_mhq_head_id head_id, const std::vector<pud_mhcs_head_id>& candidate_ids);
    void unlink_head(pud_mhq_head_id head_id);

    IMakeHead& make_head_;
    IForkHead& fork_head_;

    pud_mhq_head_id next_head_id_;
    std::unordered_map<pud_mhq_head_id, Head> heads_;
    std::unordered_map<pud_mhq_head_id, std::unordered_set<pud_mhcs_head_id>> head_to_candidates_;
    std::unordered_map<pud_mhcs_head_id, pud_mhq_head_id> candidate_to_head_;
};

#define PUD_MHQ_TEMPLATE                                                                   \
    template<typename ReplayDescent, typename Head, typename IMakeHead, typename IForkHead>
#define PUD_MHQ pud_mhq<ReplayDescent, Head, IMakeHead, IForkHead>

PUD_MHQ_TEMPLATE
PUD_MHQ::pud_mhq(IMakeHead& make_head, IForkHead& fork_head)
    : make_head_(make_head)
    , fork_head_(fork_head)
    , next_head_id_(0) {}

PUD_MHQ_TEMPLATE
pud_mhq_head_admission PUD_MHQ::try_add_head(
    const std::vector<ReplayDescent>& root_descents) {
    const pud_mhq_head_id head_id = next_head_id_++;
    auto [head_it, head_inserted] = heads_.emplace(head_id, make_head_.make(root_descents));
    DEBUG_ASSERT(head_inserted);
    return try_keep_head(head_id, head_it->second.resume());
}

PUD_MHQ_TEMPLATE
void PUD_MHQ::remove_head(pud_mhq_head_id head_id) {
    if (!heads_.contains(head_id))
        return;
    unlink_head(head_id);
    heads_.erase(head_id);
}

PUD_MHQ_TEMPLATE
std::optional<pud_forced_query_propagation> PUD_MHQ::candidate_specialization_refuted(
    pud_mhcs_head_id candidate_head_id) {
    auto candidate_it = candidate_to_head_.find(candidate_head_id);
    if (candidate_it == candidate_to_head_.end())
        return std::nullopt;

    const pud_mhq_head_id head_id = candidate_it->second;
    candidate_to_head_.erase(candidate_it);
    head_to_candidates_.at(head_id).erase(candidate_head_id);

    std::optional<pud_forced_unfold_reason> reason = heads_.at(head_id).candidate_refuted(candidate_head_id);
    if (!reason.has_value())
        return std::nullopt;

    const bool refuted = reason.value() == pud_forced_unfold_reason::refuted;
    if (refuted) {
        unlink_head(head_id);
        heads_.erase(head_id);
        return pud_refuted_query{.head_id = head_id};
    }
    return pud_unit_query{.head_id = head_id};
}

PUD_MHQ_TEMPLATE
pud_mhq_head_admission PUD_MHQ::try_fork_head(
    pud_mhq_head_id head_id,
    const std::vector<ReplayDescent>& root_descents) {
    const Head& old_head = heads_.at(head_id);
    const pud_mhq_head_id new_head_id = next_head_id_++;
    auto [new_head_it, new_head_inserted] = heads_.emplace(
        new_head_id,
        fork_head_.fork(old_head, root_descents));
    DEBUG_ASSERT(new_head_inserted);
    return try_keep_head(new_head_id, new_head_it->second.resume());
}

PUD_MHQ_TEMPLATE
pud_mhq_head_admission PUD_MHQ::try_keep_head(
    pud_mhq_head_id head_id,
    const pud_query_resume_context& context) {
    const bool refuted = context.forced_unfold_reason == pud_forced_unfold_reason::refuted;
    if (refuted) {
        heads_.erase(head_id);
        return pud_mhq_head_admission{.head_id = head_id, .forced_propagation = pud_refuted_query{.head_id = head_id}};
    }
    link(head_id, context.candidate_ids);
    if (!context.forced_unfold_reason.has_value())
        return pud_mhq_head_admission{.head_id = head_id, .forced_propagation = std::nullopt};
    return pud_mhq_head_admission{.head_id = head_id, .forced_propagation = pud_unit_query{.head_id = head_id}};
}

PUD_MHQ_TEMPLATE
void PUD_MHQ::link(pud_mhq_head_id head_id, const std::vector<pud_mhcs_head_id>& candidate_ids) {
    std::unordered_set<pud_mhcs_head_id>& head_candidates = head_to_candidates_[head_id];
    for (pud_mhcs_head_id candidate_id : candidate_ids) {
        head_candidates.insert(candidate_id);
        const bool inserted = candidate_to_head_.emplace(candidate_id, head_id).second;
        DEBUG_ASSERT(inserted);
    }
}

PUD_MHQ_TEMPLATE
void PUD_MHQ::unlink_head(pud_mhq_head_id head_id) {
    auto extracted = head_to_candidates_.extract(head_id);
    for (pud_mhcs_head_id candidate_id : extracted.mapped())
        candidate_to_head_.erase(candidate_id);
}

#undef PUD_MHQ
#undef PUD_MHQ_TEMPLATE

#endif
