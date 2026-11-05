#ifndef PUD_CANDIDATE_TRAIL_RESUMER_HPP
#define PUD_CANDIDATE_TRAIL_RESUMER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_trail.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_specialization_state.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/pud_witness_search_stack.hpp"
#include "debug_assert.hpp"

// Resumes a candidate from its trail under the caller's current bindings. The candidate's path and its witnesses'
// search stacks are rebuilt from parent links and replayed. The specialization then continues forward from where it
// stood: what it passed over before still fails, since bindings only get more specific and refutation is permanent.
template<
    typename IEnterReplay,
    typename IDescendReplay,
    typename IFindAncestorPath,
    typename IRebuildWitnessSearch,
    typename IResumeWitnessSearch,
    typename ISpecializeCandidate,
    typename IGetChildren>
struct pud_candidate_trail_resumer {
    pud_candidate_trail_resumer(
        IEnterReplay& enter_replay,
        IDescendReplay& descend_replay,
        IFindAncestorPath& find_ancestor_path,
        IRebuildWitnessSearch& rebuild_witness_search,
        IResumeWitnessSearch& resume_witness_search,
        ISpecializeCandidate& specialize_candidate,
        IGetChildren& get_children);
    std::optional<pud_eager_candidate> resume(
        pud_module_id module,
        const pud_ubs_state& caller_bindings,
        const expr* goal,
        uint32_t frame_offset,
        const pud_candidate_trail& trail) const;
private:
    size_t find_child_index(pud_global_node_id parent, pud_global_node_id child) const;
    std::optional<pud_specialization_witness> resume_witness(
        const pud_replay& candidate, const std::vector<pud_global_node_id>& chain) const;

    IEnterReplay& enter_replay_;
    IDescendReplay& descend_replay_;
    IFindAncestorPath& find_ancestor_path_;
    IRebuildWitnessSearch& rebuild_witness_search_;
    IResumeWitnessSearch& resume_witness_search_;
    ISpecializeCandidate& specialize_candidate_;
    IGetChildren& get_children_;
};

#define PUD_CANDIDATE_TRAIL_RESUMER_TEMPLATE                                                             \
    template<typename IEnterReplay, typename IDescendReplay, typename IFindAncestorPath,                 \
             typename IRebuildWitnessSearch, typename IResumeWitnessSearch, typename ISpecializeCandidate, \
             typename IGetChildren>
#define PUD_CANDIDATE_TRAIL_RESUMER                                                                      \
    pud_candidate_trail_resumer<IEnterReplay, IDescendReplay, IFindAncestorPath, IRebuildWitnessSearch,  \
        IResumeWitnessSearch, ISpecializeCandidate, IGetChildren>

PUD_CANDIDATE_TRAIL_RESUMER_TEMPLATE
PUD_CANDIDATE_TRAIL_RESUMER::pud_candidate_trail_resumer(
    IEnterReplay& enter_replay,
    IDescendReplay& descend_replay,
    IFindAncestorPath& find_ancestor_path,
    IRebuildWitnessSearch& rebuild_witness_search,
    IResumeWitnessSearch& resume_witness_search,
    ISpecializeCandidate& specialize_candidate,
    IGetChildren& get_children)
    : enter_replay_(enter_replay)
    , descend_replay_(descend_replay)
    , find_ancestor_path_(find_ancestor_path)
    , rebuild_witness_search_(rebuild_witness_search)
    , resume_witness_search_(resume_witness_search)
    , specialize_candidate_(specialize_candidate)
    , get_children_(get_children) {}

// Replays down the candidate's path. Where a node no longer replays, the candidate stands on its parent and looks for
// witnesses after it; otherwise it stands where it was, with its witnesses resumed.
PUD_CANDIDATE_TRAIL_RESUMER_TEMPLATE
std::optional<pud_eager_candidate> PUD_CANDIDATE_TRAIL_RESUMER::resume(
    pud_module_id module,
    const pud_ubs_state& caller_bindings,
    const expr* goal,
    uint32_t frame_offset,
    const pud_candidate_trail& trail) const {
    const std::vector<pud_global_node_id> path = find_ancestor_path_.find_path_from_root(trail.candidate);
    std::optional<pud_replay> axiom_root =
        enter_replay_.enter(caller_bindings, goal, frame_offset, path.front(), module);
    if (!axiom_root.has_value())
        return std::nullopt;
    pud_specialization_state state{
        .path = {path.front()}, .current = std::move(axiom_root.value()), .witnesses = std::nullopt};
    bool path_replayed = true;
    for (size_t depth = 1; depth < path.size(); ++depth) {
        std::optional<pud_replay> next = descend_replay_.descend(state.current, path.at(depth));
        if (!next.has_value()) {
            const size_t next_child = find_child_index(state.current.node, path.at(depth)) + 1;
            state.witnesses =
                pud_specialization_witnesses{.a = std::nullopt, .b = std::nullopt, .next_child = next_child};
            path_replayed = false;
            break;
        }
        state.path.push_back(path.at(depth));
        state.current = std::move(next.value());
    }
    const bool self_witnessed = trail.witness_leaves.size() == 1 && trail.witness_leaves.front() == trail.candidate;
    if (path_replayed && !self_witnessed) {
        DEBUG_ASSERT(trail.witness_leaves.size() == 2);
        const std::vector<pud_global_node_id> chain_a =
            find_ancestor_path_.find_path_below(trail.candidate, trail.witness_leaves.at(0));
        const std::vector<pud_global_node_id> chain_b =
            find_ancestor_path_.find_path_below(trail.candidate, trail.witness_leaves.at(1));
        const size_t index_a = find_child_index(trail.candidate, chain_a.front());
        const size_t index_b = find_child_index(trail.candidate, chain_b.front());
        state.witnesses = pud_specialization_witnesses{
            .a = resume_witness(state.current, chain_a),
            .b = resume_witness(state.current, chain_b),
            .next_child = std::max(index_a, index_b) + 1};
    }
    if (!specialize_candidate_.specialize(state))
        return std::nullopt;
    pud_eager_candidate found{.path = std::move(state.path), .witness_leaves = {}};
    if (!state.witnesses.has_value()) {
        found.witness_leaves.push_back(state.current.node);
        return found;
    }
    // A fresh query finds its witnesses in child order; a refilled witness may stand after the one that survived.
    const pud_specialization_witness& a = state.witnesses->a.value();
    const pud_specialization_witness& b = state.witnesses->b.value();
    const size_t a_child = find_child_index(state.current.node, a.search_root.node);
    const size_t b_child = find_child_index(state.current.node, b.search_root.node);
    const bool a_first = a_child < b_child;
    found.witness_leaves = a_first ? std::vector<pud_global_node_id>{a.leaf, b.leaf}
                                   : std::vector<pud_global_node_id>{b.leaf, a.leaf};
    return found;
}

PUD_CANDIDATE_TRAIL_RESUMER_TEMPLATE
size_t PUD_CANDIDATE_TRAIL_RESUMER::find_child_index(pud_global_node_id parent, pud_global_node_id child) const {
    const std::vector<pud_node_id>& children = get_children_.get_children(parent);
    const auto position = std::find(children.begin(), children.end(), child.node);
    DEBUG_ASSERT(position != children.end());
    return static_cast<size_t>(position - children.begin());
}

// A witness's search starts at the candidate's child the chain begins with and stood on the chain's last node.
PUD_CANDIDATE_TRAIL_RESUMER_TEMPLATE
std::optional<pud_specialization_witness> PUD_CANDIDATE_TRAIL_RESUMER::resume_witness(
    const pud_replay& candidate, const std::vector<pud_global_node_id>& chain) const {
    std::optional<pud_replay> search_root = descend_replay_.descend(candidate, chain.front());
    if (!search_root.has_value())
        return std::nullopt;
    const std::vector<pud_global_node_id> below(chain.begin() + 1, chain.end());
    pud_witness_search_stack search = rebuild_witness_search_.rebuild(search_root.value(), below);
    const std::optional<pud_global_node_id> leaf = resume_witness_search_.resume(search_root.value(), search);
    if (!leaf.has_value())
        return std::nullopt;
    return pud_specialization_witness{
        .search_root = std::move(search_root.value()), .search = std::move(search), .leaf = leaf.value()};
}

#undef PUD_CANDIDATE_TRAIL_RESUMER
#undef PUD_CANDIDATE_TRAIL_RESUMER_TEMPLATE

#endif
