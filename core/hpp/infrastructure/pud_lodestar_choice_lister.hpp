#ifndef PUD_LODESTAR_CHOICE_LISTER_HPP
#define PUD_LODESTAR_CHOICE_LISTER_HPP

#include <algorithm>
#include <limits>
#include <optional>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_candidate_witnesses.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_leaf_goal_query.hpp"
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_lodestar_side.hpp"
#include "value_objects/pud_lodestar_slot_id.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

// Lists one focus query's unfold choices in one ring, sorted by slot. A slot is a witness leaf that the
// query's candidate rests on; the focus ring keeps the witness leaves inside the conjecture module plus one
// caller slot for the query, the other ring keeps the witness leaves outside it. Only choices that can be
// unfolded now are kept: a live, unrefuted leaf that may still be unfolded and has a goal with candidates
// (its smallest such goal is the one chosen). Every slot carries the query's leaf and goal, so a ring is
// its queries' lists laid end to end in (leaf, goal) order.
template<
    typename IGetQueryCandidates,
    typename IFindCandidateWitnesses,
    typename IFindWitnessNode,
    typename IContainsFocusNode,
    typename IGetLiveNodes,
    typename IIsNodeRefuted,
    typename IGetNodeQueries,
    typename IIsQueryExhausted,
    typename IAllowUnfold>
struct pud_lodestar_choice_lister {
    pud_lodestar_choice_lister(
        IGetQueryCandidates& get_query_candidates,
        IFindCandidateWitnesses& find_candidate_witnesses,
        IFindWitnessNode& find_witness_node,
        IContainsFocusNode& contains_focus_node,
        IGetLiveNodes& get_live_nodes,
        IIsNodeRefuted& is_node_refuted,
        IGetNodeQueries& get_node_queries,
        IIsQueryExhausted& is_query_exhausted,
        IAllowUnfold& allow_unfold);
    std::vector<pud_lodestar_choice> list(const pud_leaf_goal_query& query, bool focus_ring) const;
private:
    void push_witness_slot(
        std::vector<pud_lodestar_choice>& choices,
        const pud_leaf_goal_query& query,
        pud_mhcs_head_id candidate,
        pud_lodestar_side side,
        pud_global_node_id leaf) const;

    IGetQueryCandidates& get_query_candidates_;
    IFindCandidateWitnesses& find_candidate_witnesses_;
    IFindWitnessNode& find_witness_node_;
    IContainsFocusNode& contains_focus_node_;
    IGetLiveNodes& get_live_nodes_;
    IIsNodeRefuted& is_node_refuted_;
    IGetNodeQueries& get_node_queries_;
    IIsQueryExhausted& is_query_exhausted_;
    IAllowUnfold& allow_unfold_;
};

#define PUD_LODESTAR_CHOICE_LISTER_TEMPLATE                                                          \
    template<typename IGetQueryCandidates, typename IFindCandidateWitnesses, typename IFindWitnessNode, \
             typename IContainsFocusNode, typename IGetLiveNodes, typename IIsNodeRefuted,          \
             typename IGetNodeQueries, typename IIsQueryExhausted, typename IAllowUnfold>
#define PUD_LODESTAR_CHOICE_LISTER                                                                   \
    pud_lodestar_choice_lister<IGetQueryCandidates, IFindCandidateWitnesses, IFindWitnessNode,        \
        IContainsFocusNode, IGetLiveNodes, IIsNodeRefuted, IGetNodeQueries, IIsQueryExhausted, IAllowUnfold>

PUD_LODESTAR_CHOICE_LISTER_TEMPLATE
PUD_LODESTAR_CHOICE_LISTER::pud_lodestar_choice_lister(
    IGetQueryCandidates& get_query_candidates,
    IFindCandidateWitnesses& find_candidate_witnesses,
    IFindWitnessNode& find_witness_node,
    IContainsFocusNode& contains_focus_node,
    IGetLiveNodes& get_live_nodes,
    IIsNodeRefuted& is_node_refuted,
    IGetNodeQueries& get_node_queries,
    IIsQueryExhausted& is_query_exhausted,
    IAllowUnfold& allow_unfold)
    : get_query_candidates_(get_query_candidates)
    , find_candidate_witnesses_(find_candidate_witnesses)
    , find_witness_node_(find_witness_node)
    , contains_focus_node_(contains_focus_node)
    , get_live_nodes_(get_live_nodes)
    , is_node_refuted_(is_node_refuted)
    , get_node_queries_(get_node_queries)
    , is_query_exhausted_(is_query_exhausted)
    , allow_unfold_(allow_unfold) {}

PUD_LODESTAR_CHOICE_LISTER_TEMPLATE
std::vector<pud_lodestar_choice> PUD_LODESTAR_CHOICE_LISTER::list(
    const pud_leaf_goal_query& query, bool focus_ring) const {
    std::vector<pud_lodestar_choice> choices;
    if (!query.has_candidates)
        return choices;
    for (const pud_mhcs_head_id candidate : get_query_candidates_.candidates(query.head)) {
        const pud_candidate_witnesses* witnesses_of = find_candidate_witnesses_.find(candidate);
        if (witnesses_of == nullptr)
            continue;
        if (witnesses_of->self_witness) {
            if (contains_focus_node_.contains(witnesses_of->self_leaf) == focus_ring)
                push_witness_slot(choices, query, candidate, pud_lodestar_side::self, witnesses_of->self_leaf);
            continue;
        }
        pud_lodestar_side side = pud_lodestar_side::a;
        for (const pud_mhws_head_id witness : witnesses_of->witnesses) {
            const std::optional<pud_global_node_id> leaf = find_witness_node_.node_of(witness);
            if (leaf.has_value() && contains_focus_node_.contains(leaf.value()) == focus_ring)
                push_witness_slot(choices, query, candidate, side, leaf.value());
            side = pud_lodestar_side::b;
        }
    }
    const bool caller_slot = focus_ring && allow_unfold_.allow(query.node);
    if (caller_slot)
        choices.push_back(pud_lodestar_choice{
            .slot = pud_lodestar_slot_id{
                .query_node = query.node,
                .query_goal = query.goal,
                .candidate = std::numeric_limits<pud_mhcs_head_id>::max(),
                .side = pud_lodestar_side::caller,
            },
            .leaf = query.node,
            .body_goal = query.goal,
        });
    std::sort(choices.begin(), choices.end());
    return choices;
}

PUD_LODESTAR_CHOICE_LISTER_TEMPLATE
void PUD_LODESTAR_CHOICE_LISTER::push_witness_slot(
    std::vector<pud_lodestar_choice>& choices,
    const pud_leaf_goal_query& query,
    pud_mhcs_head_id candidate,
    pud_lodestar_side side,
    pud_global_node_id leaf) const {
    if (!get_live_nodes_.live().contains(leaf))
        return;
    if (is_node_refuted_.is_refuted(leaf))
        return;
    std::optional<pud_body_goal_id> smallest_ready_goal;
    for (const auto& [goal, head] : get_node_queries_.get(leaf)) {
        if (is_query_exhausted_.is_exhausted(head))
            continue;
        if (smallest_ready_goal.has_value() && goal >= smallest_ready_goal.value())
            continue;
        smallest_ready_goal = goal;
    }
    if (!smallest_ready_goal.has_value())
        return;
    if (!allow_unfold_.allow(leaf))
        return;
    choices.push_back(pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = query.node,
            .query_goal = query.goal,
            .candidate = candidate,
            .side = side,
        },
        .leaf = leaf,
        .body_goal = smallest_ready_goal.value(),
    });
}

#undef PUD_LODESTAR_CHOICE_LISTER
#undef PUD_LODESTAR_CHOICE_LISTER_TEMPLATE

#endif
