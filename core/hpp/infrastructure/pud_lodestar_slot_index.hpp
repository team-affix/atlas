#ifndef PUD_LODESTAR_SLOT_INDEX_HPP
#define PUD_LODESTAR_SLOT_INDEX_HPP

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_candidate_witnesses.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"
#include "value_objects/pud_introduced_query.hpp"
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_lodestar_side.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "debug_assert.hpp"

// Keeps Lodestar's two rings as sorted sets of choices, updated as the search changes instead of rebuilt.
// Every frontier mutation is forwarded, then the queries introduced and withdrawn since are applied, then the nodes
// it touched are settled, together with the witness searches that may have moved since: those of each invalidated
// leaf, and each search advanced or removed.
//
// Each live query introduced at a focus node opens entries once, however many leaves inherit it: one per witness
// search its candidates were recorded with (side a for the first, b after), or one for a candidate that is its
// own witness. The entries close when the query is withdrawn. An entry rests on the leaf its search or candidate is
// at now and holds a choice while that leaf is live, unrefuted, may be unfolded and has an open goal; the choice
// unfolds that leaf and sits in the ring of the leaf's side of the focus. Each live, unrefuted focus leaf that may
// be unfolded and has an open goal also holds one caller choice in the conjecture ring, which unfolds it. Which
// goal a dealt leaf is unfolded on is chosen when it is unfolded, not here.
//
// Entries are grouped by the witness search they follow, and each search's leaf is kept once for its group. A
// touched node opens or closes its caller choice as it joins or leaves the live focus, and replaces the choices of
// every entry resting on it. An unsettled search is reread once, and its group moves only if it left its leaf, so
// a search that stayed put costs one lookup however many entries follow it.
template<
    typename IApplyRegister,
    typename IApplyUnfold,
    typename IEraseRefuted,
    typename IEraseLeaf,
    typename ITakeUnsettledLeaves,
    typename ITakeUnsettledSearches,
    typename IGetLiveFocusLeaves,
    typename IGetLiveNodes,
    typename IIsNodeRefuted,
    typename IContainsFocusNode,
    typename IGetNodeGoals,
    typename IGetQueryCandidates,
    typename IFindCandidateWitnesses,
    typename IFindWitnessNode,
    typename IAllowUnfold,
    typename ITakeIntroducedQueries,
    typename ITakeWithdrawnQueries>
struct pud_lodestar_slot_index {
    pud_lodestar_slot_index(
        IApplyRegister& apply_register,
        IApplyUnfold& apply_unfold,
        IEraseRefuted& erase_refuted,
        IEraseLeaf& erase_leaf,
        ITakeUnsettledLeaves& take_unsettled_leaves,
        ITakeUnsettledSearches& take_unsettled_searches,
        IGetLiveFocusLeaves& get_live_focus_leaves,
        IGetLiveNodes& get_live_nodes,
        IIsNodeRefuted& is_node_refuted,
        IContainsFocusNode& contains_focus_node,
        IGetNodeGoals& get_node_goals,
        IGetQueryCandidates& get_query_candidates,
        IFindCandidateWitnesses& find_candidate_witnesses,
        IFindWitnessNode& find_witness_node,
        IAllowUnfold& allow_unfold,
        ITakeIntroducedQueries& take_introduced_queries,
        ITakeWithdrawnQueries& take_withdrawn_queries);
    void apply_register(const pud_frontier_change& change);
    void apply_unfold(pud_global_node_id parent, const pud_frontier_change& change);
    void erase_refuted(const std::vector<pud_global_node_id>& refuted);
    void erase(pud_global_node_id node);
    const std::multiset<pud_lodestar_choice>& ring(bool focus_ring) const;
private:
    struct entry {
        pud_global_node_id query_node;
        pud_body_goal_id query_goal;
        pud_mhcs_head_id candidate;
        pud_lodestar_side side;
        std::optional<pud_mhws_head_id> witness;
        pud_global_node_id self_leaf;
        std::optional<pud_lodestar_choice> placed;
        bool placed_in_focus_ring;
    };
    struct witness_group {
        std::optional<pud_global_node_id> leaf;
        std::unordered_set<size_t> entries;
    };

    void settle(const std::vector<pud_global_node_id>& touched);
    void open_query(const pud_introduced_query& introduced);
    void close_query(pud_mhq_head_id head);
    void follow_leaf(pud_global_node_id leaf);
    void follow_search(pud_mhws_head_id witness);
    void sync(pud_global_node_id node);
    void open(pud_global_node_id leaf);
    void close(pud_global_node_id leaf);
    void add_entry(pud_mhq_head_id head, const entry& added);
    void remove_entry(size_t entry_id);
    void move_group(pud_mhws_head_id witness, witness_group& moved, std::optional<pud_global_node_id> leaf);
    void place(size_t entry_id);
    void unplace(entry& placed);
    bool has_open_goal(pud_global_node_id leaf) const;
    std::multiset<pud_lodestar_choice>& ring_for(bool focus_ring);

    IApplyRegister& apply_register_;
    IApplyUnfold& apply_unfold_;
    IEraseRefuted& erase_refuted_;
    IEraseLeaf& erase_leaf_;
    ITakeUnsettledLeaves& take_unsettled_leaves_;
    ITakeUnsettledSearches& take_unsettled_searches_;
    IGetLiveFocusLeaves& get_live_focus_leaves_;
    IGetLiveNodes& get_live_nodes_;
    IIsNodeRefuted& is_node_refuted_;
    IContainsFocusNode& contains_focus_node_;
    IGetNodeGoals& get_node_goals_;
    IGetQueryCandidates& get_query_candidates_;
    IFindCandidateWitnesses& find_candidate_witnesses_;
    IFindWitnessNode& find_witness_node_;
    IAllowUnfold& allow_unfold_;
    ITakeIntroducedQueries& take_introduced_queries_;
    ITakeWithdrawnQueries& take_withdrawn_queries_;
    size_t next_entry_id_;
    std::unordered_map<size_t, entry> entries_;
    std::unordered_map<pud_mhq_head_id, std::vector<size_t>> query_entries_;
    std::unordered_map<pud_global_node_id, std::optional<pud_lodestar_choice>, pud_global_node_id_hash> open_leaves_;
    std::unordered_map<pud_mhws_head_id, witness_group> witness_groups_;
    std::unordered_map<pud_global_node_id, std::unordered_set<pud_mhws_head_id>, pud_global_node_id_hash>
        resting_witnesses_;
    std::unordered_map<pud_global_node_id, std::unordered_set<size_t>, pud_global_node_id_hash> self_entries_;
    std::multiset<pud_lodestar_choice> conjecture_ring_;
    std::multiset<pud_lodestar_choice> non_conjecture_ring_;
};

#define PUD_LODESTAR_SLOT_INDEX_TEMPLATE                                                                 \
    template<typename IApplyRegister, typename IApplyUnfold, typename IEraseRefuted, typename IEraseLeaf,  \
             typename ITakeUnsettledLeaves, typename ITakeUnsettledSearches, typename IGetLiveFocusLeaves,  \
             typename IGetLiveNodes, typename IIsNodeRefuted, typename IContainsFocusNode, typename IGetNodeGoals,   \
             typename IGetQueryCandidates, typename IFindCandidateWitnesses,   \
             typename IFindWitnessNode, typename IAllowUnfold, typename ITakeIntroducedQueries,           \
             typename ITakeWithdrawnQueries>
#define PUD_LODESTAR_SLOT_INDEX                                                                          \
    pud_lodestar_slot_index<IApplyRegister, IApplyUnfold, IEraseRefuted, IEraseLeaf, ITakeUnsettledLeaves, \
        ITakeUnsettledSearches, IGetLiveFocusLeaves, IGetLiveNodes, IIsNodeRefuted, IContainsFocusNode,    \
        IGetNodeGoals, IGetQueryCandidates, IFindCandidateWitnesses, IFindWitnessNode,   \
        IAllowUnfold, ITakeIntroducedQueries, ITakeWithdrawnQueries>

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
PUD_LODESTAR_SLOT_INDEX::pud_lodestar_slot_index(
    IApplyRegister& apply_register,
    IApplyUnfold& apply_unfold,
    IEraseRefuted& erase_refuted,
    IEraseLeaf& erase_leaf,
    ITakeUnsettledLeaves& take_unsettled_leaves,
    ITakeUnsettledSearches& take_unsettled_searches,
    IGetLiveFocusLeaves& get_live_focus_leaves,
    IGetLiveNodes& get_live_nodes,
    IIsNodeRefuted& is_node_refuted,
    IContainsFocusNode& contains_focus_node,
    IGetNodeGoals& get_node_goals,
    IGetQueryCandidates& get_query_candidates,
    IFindCandidateWitnesses& find_candidate_witnesses,
    IFindWitnessNode& find_witness_node,
    IAllowUnfold& allow_unfold,
    ITakeIntroducedQueries& take_introduced_queries,
    ITakeWithdrawnQueries& take_withdrawn_queries)
    : apply_register_(apply_register)
    , apply_unfold_(apply_unfold)
    , erase_refuted_(erase_refuted)
    , erase_leaf_(erase_leaf)
    , take_unsettled_leaves_(take_unsettled_leaves)
    , take_unsettled_searches_(take_unsettled_searches)
    , get_live_focus_leaves_(get_live_focus_leaves)
    , get_live_nodes_(get_live_nodes)
    , is_node_refuted_(is_node_refuted)
    , contains_focus_node_(contains_focus_node)
    , get_node_goals_(get_node_goals)
    , get_query_candidates_(get_query_candidates)
    , find_candidate_witnesses_(find_candidate_witnesses)
    , find_witness_node_(find_witness_node)
    , allow_unfold_(allow_unfold)
    , take_introduced_queries_(take_introduced_queries)
    , take_withdrawn_queries_(take_withdrawn_queries)
    , next_entry_id_(0)
    , entries_()
    , query_entries_()
    , open_leaves_()
    , witness_groups_()
    , resting_witnesses_()
    , self_entries_()
    , conjecture_ring_()
    , non_conjecture_ring_() {}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::apply_register(const pud_frontier_change& change) {
    apply_register_.apply_register(change);
    std::vector<pud_global_node_id> touched = change.offspring;
    touched.insert(touched.end(), change.refuted.begin(), change.refuted.end());
    settle(touched);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::apply_unfold(pud_global_node_id parent, const pud_frontier_change& change) {
    apply_unfold_.apply_unfold(parent, change);
    std::vector<pud_global_node_id> touched = change.offspring;
    touched.insert(touched.end(), change.refuted.begin(), change.refuted.end());
    touched.push_back(parent);
    settle(touched);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::erase_refuted(const std::vector<pud_global_node_id>& refuted) {
    erase_refuted_.erase_refuted(refuted);
    settle(refuted);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::erase(pud_global_node_id node) {
    erase_leaf_.erase(node);
    settle({node});
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
const std::multiset<pud_lodestar_choice>& PUD_LODESTAR_SLOT_INDEX::ring(bool focus_ring) const {
    return focus_ring ? conjecture_ring_ : non_conjecture_ring_;
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::settle(const std::vector<pud_global_node_id>& touched) {
    // A query withdrawn in the same step it was introduced is gone already, so it opens nothing.
    std::vector<pud_mhq_head_id> withdrawn = take_withdrawn_queries_.take_withdrawn();
    std::sort(withdrawn.begin(), withdrawn.end());
    for (const pud_introduced_query& introduced : take_introduced_queries_.take_introduced()) {
        if (std::binary_search(withdrawn.begin(), withdrawn.end(), introduced.head))
            continue;
        open_query(introduced);
    }
    for (const pud_mhq_head_id head : withdrawn)
        close_query(head);
    std::vector<pud_global_node_id> unsettled = take_unsettled_leaves_.take_unsettled_leaves();
    std::sort(unsettled.begin(), unsettled.end());
    unsettled.erase(std::unique(unsettled.begin(), unsettled.end()), unsettled.end());
    for (const pud_global_node_id leaf : unsettled)
        follow_leaf(leaf);
    for (const pud_mhws_head_id witness : take_unsettled_searches_.take_unsettled_searches())
        follow_search(witness);
    for (const pud_global_node_id node : touched)
        sync(node);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::follow_leaf(pud_global_node_id leaf) {
    const auto resting = resting_witnesses_.find(leaf);
    if (resting == resting_witnesses_.end())
        return;
    const std::vector<pud_mhws_head_id> witnesses(resting->second.begin(), resting->second.end());
    for (const pud_mhws_head_id witness : witnesses)
        follow_search(witness);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::follow_search(pud_mhws_head_id witness) {
    const auto group = witness_groups_.find(witness);
    if (group == witness_groups_.end())
        return;
    const std::optional<pud_global_node_id> now = find_witness_node_.node_of(witness);
    if (now == group->second.leaf)
        return;
    move_group(witness, group->second, now);
    for (const size_t entry_id : group->second.entries)
        place(entry_id);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::sync(pud_global_node_id node) {
    const bool open_focus_leaf = get_live_focus_leaves_.live().contains(node) && !is_node_refuted_.is_refuted(node);
    const bool opened = open_leaves_.contains(node);
    if (open_focus_leaf && !opened)
        open(node);
    if (!open_focus_leaf && opened)
        close(node);
    const auto resting = resting_witnesses_.find(node);
    if (resting != resting_witnesses_.end()) {
        for (const pud_mhws_head_id witness : resting->second) {
            for (const size_t entry_id : witness_groups_.at(witness).entries)
                place(entry_id);
        }
    }
    const auto selves = self_entries_.find(node);
    if (selves == self_entries_.end())
        return;
    for (const size_t entry_id : selves->second)
        place(entry_id);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::open(pud_global_node_id leaf) {
    std::optional<pud_lodestar_choice>& caller = open_leaves_[leaf];
    if (!allow_unfold_.allow(leaf))
        return;
    if (!has_open_goal(leaf))
        return;
    caller = pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = leaf,
            .query_goal = std::numeric_limits<pud_body_goal_id>::max(),
            .candidate = std::numeric_limits<pud_mhcs_head_id>::max(),
            .side = pud_lodestar_side::caller,
        },
        .leaf = leaf,
    };
    conjecture_ring_.insert(caller.value());
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::close(pud_global_node_id leaf) {
    auto closed = open_leaves_.extract(leaf);
    if (!closed.mapped().has_value())
        return;
    conjecture_ring_.erase(conjecture_ring_.find(closed.mapped().value()));
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::open_query(const pud_introduced_query& introduced) {
    if (!contains_focus_node_.contains(introduced.node))
        return;
    for (const pud_mhcs_head_id candidate : get_query_candidates_.candidates(introduced.head)) {
        const pud_candidate_witnesses* rests_on = find_candidate_witnesses_.find(candidate);
        if (rests_on == nullptr)
            continue;
        entry added{
            .query_node = introduced.node,
            .query_goal = introduced.goal,
            .candidate = candidate,
            .side = pud_lodestar_side::self,
            .witness = std::nullopt,
            .self_leaf = rests_on->self_leaf,
            .placed = std::nullopt,
            .placed_in_focus_ring = false,
        };
        if (rests_on->self_witness) {
            add_entry(introduced.head, added);
            continue;
        }
        added.side = pud_lodestar_side::a;
        for (const pud_mhws_head_id witness : rests_on->witnesses) {
            added.witness = witness;
            add_entry(introduced.head, added);
            added.side = pud_lodestar_side::b;
        }
    }
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::close_query(pud_mhq_head_id head) {
    auto closed = query_entries_.extract(head);
    if (closed.empty())
        return;
    for (const size_t entry_id : closed.mapped())
        remove_entry(entry_id);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::add_entry(pud_mhq_head_id head, const entry& added) {
    const size_t entry_id = next_entry_id_++;
    entries_.emplace(entry_id, added);
    query_entries_[head].push_back(entry_id);
    if (!added.witness.has_value()) {
        self_entries_[added.self_leaf].insert(entry_id);
        place(entry_id);
        return;
    }
    const pud_mhws_head_id witness = added.witness.value();
    auto [group, created] = witness_groups_.try_emplace(witness, witness_group{.leaf = std::nullopt, .entries = {}});
    if (created)
        move_group(witness, group->second, find_witness_node_.node_of(witness));
    group->second.entries.insert(entry_id);
    place(entry_id);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::remove_entry(size_t entry_id) {
    entry& removed = entries_.at(entry_id);
    unplace(removed);
    if (!removed.witness.has_value()) {
        const auto selves = self_entries_.find(removed.self_leaf);
        selves->second.erase(entry_id);
        if (selves->second.empty())
            self_entries_.erase(selves);
        entries_.erase(entry_id);
        return;
    }
    const pud_mhws_head_id witness = removed.witness.value();
    entries_.erase(entry_id);
    witness_group& group = witness_groups_.at(witness);
    group.entries.erase(entry_id);
    if (!group.entries.empty())
        return;
    move_group(witness, group, std::nullopt);
    witness_groups_.erase(witness);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::move_group(
    pud_mhws_head_id witness, witness_group& moved, std::optional<pud_global_node_id> leaf) {
    if (moved.leaf.has_value()) {
        const auto resting = resting_witnesses_.find(moved.leaf.value());
        resting->second.erase(witness);
        if (resting->second.empty())
            resting_witnesses_.erase(resting);
    }
    moved.leaf = leaf;
    if (leaf.has_value())
        resting_witnesses_[leaf.value()].insert(witness);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::place(size_t entry_id) {
    entry& placed = entries_.at(entry_id);
    unplace(placed);
    const std::optional<pud_global_node_id> rests_on = placed.witness.has_value()
        ? witness_groups_.at(placed.witness.value()).leaf
        : std::optional<pud_global_node_id>(placed.self_leaf);
    if (!rests_on.has_value())
        return;
    const pud_global_node_id leaf = rests_on.value();
    if (!get_live_nodes_.live().contains(leaf))
        return;
    if (is_node_refuted_.is_refuted(leaf))
        return;
    if (!has_open_goal(leaf))
        return;
    if (!allow_unfold_.allow(leaf))
        return;
    const pud_lodestar_choice choice{
        .slot = pud_lodestar_slot_id{
            .query_node = placed.query_node,
            .query_goal = placed.query_goal,
            .candidate = placed.candidate,
            .side = placed.side,
        },
        .leaf = leaf,
    };
    placed.placed = choice;
    placed.placed_in_focus_ring = contains_focus_node_.contains(leaf);
    ring_for(placed.placed_in_focus_ring).insert(choice);
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
void PUD_LODESTAR_SLOT_INDEX::unplace(entry& placed) {
    if (!placed.placed.has_value())
        return;
    std::multiset<pud_lodestar_choice>& ring = ring_for(placed.placed_in_focus_ring);
    ring.erase(ring.find(placed.placed.value()));
    placed.placed = std::nullopt;
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
bool PUD_LODESTAR_SLOT_INDEX::has_open_goal(pud_global_node_id leaf) const {
    return !get_node_goals_.get(leaf).goals.empty();
}

PUD_LODESTAR_SLOT_INDEX_TEMPLATE
std::multiset<pud_lodestar_choice>& PUD_LODESTAR_SLOT_INDEX::ring_for(bool focus_ring) {
    return focus_ring ? conjecture_ring_ : non_conjecture_ring_;
}

#undef PUD_LODESTAR_SLOT_INDEX
#undef PUD_LODESTAR_SLOT_INDEX_TEMPLATE

#endif
