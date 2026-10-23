#ifndef PUD_MHCS_HPP
#define PUD_MHCS_HPP

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_advance.hpp"

// The multi-headed candidate search. Each head specializes its query below one axiom as deep as its
// witnesses allow: a head on a live leaf is its own witness; otherwise it holds two witness searches
// under distinct children (a choice point), and when only one survives it steps down toward it.
// Heads are reactive: invalidating leaves re-justifies every head that relied on them, and a head
// that can no longer be justified is refuted and dropped.
template<
    typename IDescendReplay,
    typename ITryAddWitnessSearch,
    typename ITryForkWitnessSearch,
    typename IAdvanceWitnessSearch,
    typename IRemoveWitnessSearch,
    typename IInvalidateWitnessLeaf,
    typename ICheckNodeLeaf,
    typename ICheckNodeRefuted,
    typename IGetChildren>
struct pud_mhcs {
    pud_mhcs(
        IDescendReplay& descend_replay,
        ITryAddWitnessSearch& try_add_witness_search,
        ITryForkWitnessSearch& try_fork_witness_search,
        IAdvanceWitnessSearch& advance_witness_search,
        IRemoveWitnessSearch& remove_witness_search,
        IInvalidateWitnessLeaf& invalidate_witness_leaf,
        ICheckNodeLeaf& check_node_leaf,
        ICheckNodeRefuted& check_node_refuted,
        IGetChildren& get_children);
    std::optional<pud_mhcs_head_id> try_add(pud_replay axiom_root);
    std::optional<pud_mhcs_head_id> try_fork(pud_mhcs_head_id head_id, pud_replay axiom_root);
    void remove(pud_mhcs_head_id head_id);
    std::vector<pud_mhcs_head_id> invalidate_leaf(pud_node_id leaf);
    const std::vector<pud_node_id>& path(pud_mhcs_head_id head_id) const;
private:
    // A witness search and the child of the candidate it searches under.
    struct witness {
        pud_mhws_head_id id;
        pud_replay search_root;
    };
    // The candidate's search for two witnesses among its node's children, the next child to try.
    struct witness_pair {
        std::optional<witness> a;
        std::optional<witness> b;
        size_t next_child;
    };
    // A candidate holding a witness pair is a choice point; one without is its own witness.
    struct candidate {
        std::vector<pud_node_id> path;
        pud_replay current;
        std::optional<witness_pair> witnesses;
    };

    std::optional<pud_mhcs_head_id> try_admit(candidate admitted);
    bool try_justify(candidate& justified);
    std::optional<witness> try_add_witness(candidate& searching);
    std::optional<witness> try_fork_witness(const candidate& forked, const std::optional<witness>& other);
    void advance(candidate& advancing, witness& survivor);
    void release_witnesses(candidate& released);
    void link(pud_mhcs_head_id head_id, const candidate& linked);
    void unlink(pud_mhcs_head_id head_id, const candidate& linked);

    IDescendReplay& descend_replay_;
    ITryAddWitnessSearch& try_add_witness_search_;
    ITryForkWitnessSearch& try_fork_witness_search_;
    IAdvanceWitnessSearch& advance_witness_search_;
    IRemoveWitnessSearch& remove_witness_search_;
    IInvalidateWitnessLeaf& invalidate_witness_leaf_;
    ICheckNodeLeaf& check_node_leaf_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetChildren& get_children_;

    pud_mhcs_head_id next_head_id_;
    std::unordered_map<pud_mhcs_head_id, candidate> candidates_;
    std::unordered_map<pud_mhws_head_id, pud_mhcs_head_id> witness_to_head_;
    std::unordered_map<pud_node_id, std::unordered_set<pud_mhcs_head_id>> leaf_to_heads_;
};

#define PUD_MHCS_TEMPLATE                                                                  \
    template<typename IDescendReplay, typename ITryAddWitnessSearch,                       \
             typename ITryForkWitnessSearch, typename IAdvanceWitnessSearch,               \
             typename IRemoveWitnessSearch, typename IInvalidateWitnessLeaf,             \
             typename ICheckNodeLeaf, typename ICheckNodeRefuted, typename IGetChildren>
#define PUD_MHCS                                                                           \
    pud_mhcs<IDescendReplay, ITryAddWitnessSearch, ITryForkWitnessSearch,                  \
        IAdvanceWitnessSearch, IRemoveWitnessSearch, IInvalidateWitnessLeaf,             \
        ICheckNodeLeaf, ICheckNodeRefuted, IGetChildren>

PUD_MHCS_TEMPLATE
PUD_MHCS::pud_mhcs(
    IDescendReplay& descend_replay,
    ITryAddWitnessSearch& try_add_witness_search,
    ITryForkWitnessSearch& try_fork_witness_search,
    IAdvanceWitnessSearch& advance_witness_search,
    IRemoveWitnessSearch& remove_witness_search,
    IInvalidateWitnessLeaf& invalidate_witness_leaf,
    ICheckNodeLeaf& check_node_leaf,
    ICheckNodeRefuted& check_node_refuted,
    IGetChildren& get_children)
    : descend_replay_(descend_replay)
    , try_add_witness_search_(try_add_witness_search)
    , try_fork_witness_search_(try_fork_witness_search)
    , advance_witness_search_(advance_witness_search)
    , remove_witness_search_(remove_witness_search)
    , invalidate_witness_leaf_(invalidate_witness_leaf)
    , check_node_leaf_(check_node_leaf)
    , check_node_refuted_(check_node_refuted)
    , get_children_(get_children)
    , next_head_id_(0)
    , candidates_()
    , witness_to_head_()
    , leaf_to_heads_() {}

PUD_MHCS_TEMPLATE
std::optional<pud_mhcs_head_id> PUD_MHCS::try_add(pud_replay axiom_root) {
    std::vector<pud_node_id> path{axiom_root.node};
    return try_admit(candidate{.path = std::move(path), .current = std::move(axiom_root), .witnesses = std::nullopt});
}

// Replays the other head's path from the new axiom root; a path that no longer replays refutes the
// fork. A replayed choice point forks both of its witness searches onto the new environment.
PUD_MHCS_TEMPLATE
std::optional<pud_mhcs_head_id> PUD_MHCS::try_fork(pud_mhcs_head_id head_id, pud_replay axiom_root) {
    const candidate& other = candidates_.at(head_id);
    candidate forked{.path = {axiom_root.node}, .current = std::move(axiom_root), .witnesses = std::nullopt};
    for (size_t path_index = 1; path_index < other.path.size(); ++path_index) {
        std::optional<pud_replay> next = descend_replay_.descend(forked.current, other.path.at(path_index));
        if (!next.has_value())
            return std::nullopt;
        forked.path.push_back(next->node);
        forked.current = std::move(next.value());
    }
    if (other.witnesses.has_value()) {
        forked.witnesses = witness_pair{
            .a = try_fork_witness(forked, other.witnesses->a),
            .b = try_fork_witness(forked, other.witnesses->b),
            .next_child = other.witnesses->next_child,
        };
    }
    return try_admit(std::move(forked));
}

PUD_MHCS_TEMPLATE
void PUD_MHCS::remove(pud_mhcs_head_id head_id) {
    candidate& removed = candidates_.at(head_id);
    unlink(head_id, removed);
    release_witnesses(removed);
    candidates_.erase(head_id);
}

PUD_MHCS_TEMPLATE
std::vector<pud_mhcs_head_id> PUD_MHCS::invalidate_leaf(pud_node_id leaf) {
    const std::vector<pud_mhws_head_id> exhausted = invalidate_witness_leaf_.invalidate_leaf(leaf);
    std::vector<pud_mhcs_head_id> unjustified;
    auto self_witnessed = leaf_to_heads_.extract(leaf);
    if (!self_witnessed.empty())
        unjustified.insert(unjustified.end(), self_witnessed.mapped().begin(), self_witnessed.mapped().end());
    std::vector<std::pair<pud_mhcs_head_id, pud_mhws_head_id>> lost_witnesses;
    for (pud_mhws_head_id witness_id : exhausted)
        lost_witnesses.emplace_back(witness_to_head_.at(witness_id), witness_id);
    std::unordered_set<pud_mhcs_head_id> choice_points_unlinked;
    for (const auto& [head_id, witness_id] : lost_witnesses) {
        candidate& lost = candidates_.at(head_id);
        if (choice_points_unlinked.insert(head_id).second) {
            unlink(head_id, lost);
            unjustified.push_back(head_id);
        }
        std::optional<witness>& slot = lost.witnesses->a.has_value() && lost.witnesses->a->id == witness_id
            ? lost.witnesses->a
            : lost.witnesses->b;
        slot = std::nullopt;
    }
    std::vector<pud_mhcs_head_id> refuted;
    for (pud_mhcs_head_id head_id : unjustified) {
        candidate& rejustified = candidates_.at(head_id);
        if (try_justify(rejustified)) {
            link(head_id, rejustified);
            continue;
        }
        candidates_.erase(head_id);
        refuted.push_back(head_id);
    }
    return refuted;
}

// Root first, the candidate's node last.
PUD_MHCS_TEMPLATE
const std::vector<pud_node_id>& PUD_MHCS::path(pud_mhcs_head_id head_id) const {
    return candidates_.at(head_id).path;
}

PUD_MHCS_TEMPLATE
std::optional<pud_mhcs_head_id> PUD_MHCS::try_admit(candidate admitted) {
    if (!try_justify(admitted))
        return std::nullopt;
    const pud_mhcs_head_id head_id = next_head_id_++;
    link(head_id, admitted);
    candidates_.emplace(head_id, std::move(admitted));
    return head_id;
}

// A candidate that fails to be justified releases every witness search it still holds.
PUD_MHCS_TEMPLATE
bool PUD_MHCS::try_justify(candidate& justified) {
    while (true) {
        const pud_node_id node = justified.current.node;
        if (check_node_refuted_.check_refuted(node)) {
            release_witnesses(justified);
            return false;
        }
        if (check_node_leaf_.check_leaf(node)) {
            justified.witnesses = std::nullopt;
            return true;
        }
        if (!justified.witnesses.has_value())
            justified.witnesses = witness_pair{.a = std::nullopt, .b = std::nullopt, .next_child = 0};
        if (!justified.witnesses->a.has_value())
            justified.witnesses->a = try_add_witness(justified);
        if (!justified.witnesses->b.has_value())
            justified.witnesses->b = try_add_witness(justified);
        const bool has_a = justified.witnesses->a.has_value();
        const bool has_b = justified.witnesses->b.has_value();
        if (has_a && has_b)
            return true;
        if (!has_a && !has_b)
            return false;
        advance(justified, has_a ? justified.witnesses->a.value() : justified.witnesses->b.value());
    }
}

PUD_MHCS_TEMPLATE
std::optional<typename PUD_MHCS::witness> PUD_MHCS::try_add_witness(candidate& searching) {
    const std::vector<pud_node_id>& children = get_children_.get(searching.current.node);
    size_t& next_child = searching.witnesses->next_child;
    while (next_child < children.size()) {
        std::optional<pud_replay> search_root = descend_replay_.descend(searching.current, children.at(next_child++));
        if (!search_root.has_value())
            continue;
        const std::optional<pud_mhws_head_id> added = try_add_witness_search_.try_add(search_root.value());
        if (!added.has_value())
            continue;
        return witness{.id = added.value(), .search_root = std::move(search_root.value())};
    }
    return std::nullopt;
}

PUD_MHCS_TEMPLATE
std::optional<typename PUD_MHCS::witness> PUD_MHCS::try_fork_witness(
    const candidate& forked, const std::optional<witness>& other) {
    if (!other.has_value())
        return std::nullopt;
    std::optional<pud_replay> search_root = descend_replay_.descend(forked.current, other->search_root.node);
    if (!search_root.has_value())
        return std::nullopt;
    const std::optional<pud_mhws_head_id> fork = try_fork_witness_search_.try_fork(other->id, search_root.value());
    if (!fork.has_value())
        return std::nullopt;
    return witness{.id = fork.value(), .search_root = std::move(search_root.value())};
}

// Steps onto the survivor's search root, and the survivor's search moves one level down with it. A
// survivor whose search root was its witness leaves the candidate on that leaf, its own witness.
PUD_MHCS_TEMPLATE
void PUD_MHCS::advance(candidate& advancing, witness& survivor) {
    std::optional<pud_witness_advance> advanced = advance_witness_search_.advance(survivor.id);
    advancing.path.push_back(survivor.search_root.node);
    advancing.current = survivor.search_root;
    if (!advanced.has_value()) {
        advancing.witnesses = std::nullopt;
        return;
    }
    survivor.search_root = std::move(advanced->search_root);
    advancing.witnesses->next_child = advanced->next_sibling;
}

PUD_MHCS_TEMPLATE
void PUD_MHCS::release_witnesses(candidate& released) {
    if (!released.witnesses.has_value())
        return;
    if (released.witnesses->a.has_value())
        remove_witness_search_.remove(released.witnesses->a->id);
    if (released.witnesses->b.has_value())
        remove_witness_search_.remove(released.witnesses->b->id);
    released.witnesses = std::nullopt;
}

PUD_MHCS_TEMPLATE
void PUD_MHCS::link(pud_mhcs_head_id head_id, const candidate& linked) {
    if (!linked.witnesses.has_value()) {
        leaf_to_heads_[linked.current.node].insert(head_id);
        return;
    }
    witness_to_head_.emplace(linked.witnesses->a->id, head_id);
    witness_to_head_.emplace(linked.witnesses->b->id, head_id);
}

PUD_MHCS_TEMPLATE
void PUD_MHCS::unlink(pud_mhcs_head_id head_id, const candidate& linked) {
    if (linked.witnesses.has_value()) {
        witness_to_head_.erase(linked.witnesses->a->id);
        witness_to_head_.erase(linked.witnesses->b->id);
        return;
    }
    std::unordered_set<pud_mhcs_head_id>& heads = leaf_to_heads_.at(linked.current.node);
    heads.erase(head_id);
    if (heads.empty())
        leaf_to_heads_.erase(linked.current.node);
}

#undef PUD_MHCS
#undef PUD_MHCS_TEMPLATE

#endif
