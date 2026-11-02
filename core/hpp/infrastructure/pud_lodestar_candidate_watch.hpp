#ifndef PUD_LODESTAR_CANDIDATE_WATCH_HPP
#define PUD_LODESTAR_CANDIDATE_WATCH_HPP

#include <optional>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_lodestar_watched_candidate.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "debug_assert.hpp"

template<typename IMhcs, typename IWitnessLog>
struct pud_lodestar_candidate_watch {
    pud_lodestar_candidate_watch(IMhcs& inner, IWitnessLog& witnesses);
    std::optional<pud_mhcs_head_id> try_add(pud_replay axiom_root);
    std::optional<pud_mhcs_head_id> try_fork(pud_mhcs_head_id head_id, pud_replay axiom_root);
    void remove(pud_mhcs_head_id head_id);
    std::vector<pud_mhcs_head_id> invalidate_leaf(pud_global_node_id leaf);
    const pud_lodestar_watched_candidate* find(pud_mhcs_head_id head_id) const;
private:
    void record(pud_mhcs_head_id head_id, const std::vector<pud_mhws_head_id>& added);

    IMhcs& inner_;
    IWitnessLog& witnesses_;
    std::unordered_map<pud_mhcs_head_id, pud_lodestar_watched_candidate> by_candidate_;
};

template<typename IMhcs, typename IWitnessLog>
pud_lodestar_candidate_watch<IMhcs, IWitnessLog>::pud_lodestar_candidate_watch(
    IMhcs& inner, IWitnessLog& witnesses)
    : inner_(inner)
    , witnesses_(witnesses)
    , by_candidate_() {}

template<typename IMhcs, typename IWitnessLog>
void pud_lodestar_candidate_watch<IMhcs, IWitnessLog>::record(
    pud_mhcs_head_id head_id, const std::vector<pud_mhws_head_id>& added) {
    pud_lodestar_watched_candidate stored{.self_witness = added.empty(), .self_leaf = {}, .witnesses = added};
    if (stored.self_witness) {
        const std::vector<pud_global_node_id>& walked = inner_.path(head_id);
        DEBUG_ASSERT(!walked.empty());
        stored.self_leaf = walked.back();
    }
    by_candidate_[head_id] = std::move(stored);
}

template<typename IMhcs, typename IWitnessLog>
std::optional<pud_mhcs_head_id> pud_lodestar_candidate_watch<IMhcs, IWitnessLog>::try_add(pud_replay axiom_root) {
    witnesses_.begin_recording_adds();
    const std::optional<pud_mhcs_head_id> head_id = inner_.try_add(std::move(axiom_root));
    const std::vector<pud_mhws_head_id> added = witnesses_.take_recorded_adds();
    if (!head_id.has_value())
        return std::nullopt;
    record(head_id.value(), added);
    return head_id;
}

template<typename IMhcs, typename IWitnessLog>
std::optional<pud_mhcs_head_id> pud_lodestar_candidate_watch<IMhcs, IWitnessLog>::try_fork(
    pud_mhcs_head_id head_id, pud_replay axiom_root) {
    witnesses_.begin_recording_adds();
    const std::optional<pud_mhcs_head_id> forked = inner_.try_fork(head_id, std::move(axiom_root));
    const std::vector<pud_mhws_head_id> added = witnesses_.take_recorded_adds();
    if (!forked.has_value())
        return std::nullopt;
    record(forked.value(), added);
    return forked;
}

template<typename IMhcs, typename IWitnessLog>
void pud_lodestar_candidate_watch<IMhcs, IWitnessLog>::remove(pud_mhcs_head_id head_id) {
    by_candidate_.erase(head_id);
    inner_.remove(head_id);
}

template<typename IMhcs, typename IWitnessLog>
std::vector<pud_mhcs_head_id> pud_lodestar_candidate_watch<IMhcs, IWitnessLog>::invalidate_leaf(
    pud_global_node_id leaf) {
    const std::vector<pud_mhcs_head_id> refuted = inner_.invalidate_leaf(leaf);
    for (const pud_mhcs_head_id head_id : refuted)
        by_candidate_.erase(head_id);
    return refuted;
}

template<typename IMhcs, typename IWitnessLog>
const pud_lodestar_watched_candidate*
pud_lodestar_candidate_watch<IMhcs, IWitnessLog>::find(pud_mhcs_head_id head_id) const {
    const auto found = by_candidate_.find(head_id);
    if (found == by_candidate_.end())
        return nullptr;
    return &found->second;
}

#endif
