#ifndef PUD_MHQ_HPP
#define PUD_MHQ_HPP

#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_ubs_state.hpp"

// The multi-headed query. Each head is a goal's query: at most one candidate under each axiom its
// module sees, kept in axiom order. A head is admitted only with a candidate, and is reported as unit once it is down
// to one. A head whose last candidate is refuted is reported as refuted; it stays until removed.
template<
    typename IEnterReplay,
    typename IListVisibleAxioms,
    typename ITryAddCandidate,
    typename ITryForkCandidate,
    typename IRemoveCandidate,
    typename IInvalidateCandidateLeaf,
    typename IPushUnitQuery>
struct pud_mhq {
    pud_mhq(
        IEnterReplay& enter_replay,
        IListVisibleAxioms& list_visible_axioms,
        ITryAddCandidate& try_add_candidate,
        ITryForkCandidate& try_fork_candidate,
        IRemoveCandidate& remove_candidate,
        IInvalidateCandidateLeaf& invalidate_candidate_leaf,
        IPushUnitQuery& push_unit_query);
    std::optional<pud_mhq_head_id> try_open(
        pud_module_id module,
        const pud_ubs_state& caller_ubs_state,
        const expr* goal,
        uint32_t frame_offset);
    std::optional<pud_mhq_head_id> try_fork(
        pud_mhq_head_id head_id,
        const pud_ubs_state& caller_ubs_state,
        const expr* goal,
        uint32_t frame_offset);
    void remove(pud_mhq_head_id head_id);
    std::vector<pud_mhq_head_id> invalidate_leaf(pud_global_node_id leaf);
    std::vector<pud_mhcs_head_id> candidates(pud_mhq_head_id head_id) const;
    bool check_exhausted(pud_mhq_head_id head_id) const;
private:
    struct candidacy {
        pud_global_node_id axiom;
        pud_mhcs_head_id candidate;
    };

    std::optional<pud_mhq_head_id> admit_candidacy(std::optional<pud_mhq_head_id> head_id, candidacy admitted);

    IEnterReplay& enter_replay_;
    IListVisibleAxioms& list_visible_axioms_;
    ITryAddCandidate& try_add_candidate_;
    ITryForkCandidate& try_fork_candidate_;
    IRemoveCandidate& remove_candidate_;
    IInvalidateCandidateLeaf& invalidate_candidate_leaf_;
    IPushUnitQuery& push_unit_query_;

    pud_mhq_head_id next_head_id_;
    std::unordered_map<pud_mhq_head_id, std::vector<candidacy>> queries_;
    std::unordered_map<pud_mhcs_head_id, pud_mhq_head_id> candidate_to_head_;
};

#define PUD_MHQ_TEMPLATE                                                                   \
    template<typename IEnterReplay, typename IListVisibleAxioms, typename ITryAddCandidate,        \
             typename ITryForkCandidate, typename IRemoveCandidate,                        \
             typename IInvalidateCandidateLeaf, typename IPushUnitQuery>
#define PUD_MHQ                                                                            \
    pud_mhq<IEnterReplay, IListVisibleAxioms, ITryAddCandidate, ITryForkCandidate, IRemoveCandidate, \
        IInvalidateCandidateLeaf, IPushUnitQuery>

PUD_MHQ_TEMPLATE
PUD_MHQ::pud_mhq(
    IEnterReplay& enter_replay,
    IListVisibleAxioms& list_visible_axioms,
    ITryAddCandidate& try_add_candidate,
    ITryForkCandidate& try_fork_candidate,
    IRemoveCandidate& remove_candidate,
    IInvalidateCandidateLeaf& invalidate_candidate_leaf,
    IPushUnitQuery& push_unit_query)
    : enter_replay_(enter_replay)
    , list_visible_axioms_(list_visible_axioms)
    , try_add_candidate_(try_add_candidate)
    , try_fork_candidate_(try_fork_candidate)
    , remove_candidate_(remove_candidate)
    , invalidate_candidate_leaf_(invalidate_candidate_leaf)
    , push_unit_query_(push_unit_query)
    , next_head_id_(0)
    , queries_()
    , candidate_to_head_() {}

PUD_MHQ_TEMPLATE
std::optional<pud_mhq_head_id> PUD_MHQ::try_open(
    pud_module_id module,
    const pud_ubs_state& caller_ubs_state,
    const expr* goal,
    uint32_t frame_offset) {
    std::optional<pud_mhq_head_id> head_id;
    for (pud_global_node_id axiom : list_visible_axioms_.list_visible_axioms(module)) {
        std::optional<pud_replay> axiom_root = enter_replay_.enter(caller_ubs_state, goal, frame_offset, axiom);
        if (!axiom_root.has_value())
            continue;
        const std::optional<pud_mhcs_head_id> candidate = try_add_candidate_.try_add(std::move(axiom_root.value()));
        if (!candidate.has_value())
            continue;
        head_id = admit_candidacy(head_id, candidacy{.axiom = axiom, .candidate = candidate.value()});
    }
    if (!head_id.has_value())
        return std::nullopt;
    if (queries_.at(head_id.value()).size() == 1)
        push_unit_query_.push_back(head_id.value());
    return head_id;
}

// A fork only looks under the axioms the forked head still has a candidate under: a specialization
// never reaches a node the general query could not.
PUD_MHQ_TEMPLATE
std::optional<pud_mhq_head_id> PUD_MHQ::try_fork(
    pud_mhq_head_id head_id,
    const pud_ubs_state& caller_ubs_state,
    const expr* goal,
    uint32_t frame_offset) {
    std::optional<pud_mhq_head_id> forked_head_id;
    for (const candidacy& other : queries_.at(head_id)) {
        std::optional<pud_replay> axiom_root = enter_replay_.enter(caller_ubs_state, goal, frame_offset, other.axiom);
        if (!axiom_root.has_value())
            continue;
        const std::optional<pud_mhcs_head_id> candidate =
            try_fork_candidate_.try_fork(other.candidate, std::move(axiom_root.value()));
        if (!candidate.has_value())
            continue;
        forked_head_id = admit_candidacy(forked_head_id, candidacy{.axiom = other.axiom, .candidate = candidate.value()});
    }
    if (!forked_head_id.has_value())
        return std::nullopt;
    if (queries_.at(forked_head_id.value()).size() == 1)
        push_unit_query_.push_back(forked_head_id.value());
    return forked_head_id;
}

PUD_MHQ_TEMPLATE
void PUD_MHQ::remove(pud_mhq_head_id head_id) {
    for (const candidacy& removed : queries_.at(head_id)) {
        candidate_to_head_.erase(removed.candidate);
        remove_candidate_.remove(removed.candidate);
    }
    queries_.erase(head_id);
}

PUD_MHQ_TEMPLATE
std::vector<pud_mhq_head_id> PUD_MHQ::invalidate_leaf(pud_global_node_id leaf) {
    std::vector<pud_mhq_head_id> refuted;
    for (pud_mhcs_head_id candidate : invalidate_candidate_leaf_.invalidate_leaf(leaf)) {
        auto owner = candidate_to_head_.extract(candidate);
        std::vector<candidacy>& candidacies = queries_.at(owner.mapped());
        candidacies.erase(std::ranges::find(candidacies, candidate, &candidacy::candidate));
        if (candidacies.empty()) {
            refuted.push_back(owner.mapped());
            continue;
        }
        if (candidacies.size() == 1)
            push_unit_query_.push_back(owner.mapped());
    }
    return refuted;
}

PUD_MHQ_TEMPLATE
std::vector<pud_mhcs_head_id> PUD_MHQ::candidates(pud_mhq_head_id head_id) const {
    std::vector<pud_mhcs_head_id> candidate_ids;
    for (const candidacy& held : queries_.at(head_id))
        candidate_ids.push_back(held.candidate);
    return candidate_ids;
}

PUD_MHQ_TEMPLATE
bool PUD_MHQ::check_exhausted(pud_mhq_head_id head_id) const {
    return queries_.at(head_id).empty();
}

PUD_MHQ_TEMPLATE
std::optional<pud_mhq_head_id> PUD_MHQ::admit_candidacy(
    std::optional<pud_mhq_head_id> head_id, candidacy admitted) {
    if (!head_id.has_value()) {
        head_id = next_head_id_++;
        queries_.emplace(head_id.value(), std::vector<candidacy>{});
    }
    candidate_to_head_.emplace(admitted.candidate, head_id.value());
    queries_.at(head_id.value()).push_back(admitted);
    return head_id;
}

#undef PUD_MHQ
#undef PUD_MHQ_TEMPLATE

#endif
