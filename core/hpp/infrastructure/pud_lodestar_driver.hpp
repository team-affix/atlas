#ifndef PUD_LODESTAR_DRIVER_HPP
#define PUD_LODESTAR_DRIVER_HPP

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>
#include "infrastructure/pud_lodestar_frontier.hpp"
#include "infrastructure/pud_lodestar_schedule.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

template<typename IManifest>
struct pud_lodestar_driver {
    pud_lodestar_driver(
        IManifest& manifest,
        pud_lodestar_frontier& frontier,
        size_t non_conjecture_unfolds_per_conjecture_unfold);
    bool refute_blocked();
    bool unfold_one();
    size_t last_refuted() const;
    bool last_unfold_on_focus() const;
private:
    struct query_ref {
        pud_global_node_id node;
        pud_body_goal_id goal;
        pud_mhq_head_id head;
        bool has_candidates;
    };

    bool on_focus(pud_global_node_id node) const;
    std::vector<query_ref> conjecture_queries() const;
    std::optional<pud_body_goal_id> smallest_ready_goal(pud_global_node_id leaf) const;
    void push_witness_slot(
        std::vector<pud_lodestar_choice>& ring,
        const query_ref& query,
        pud_mhcs_head_id candidate,
        uint8_t side,
        pud_global_node_id leaf) const;
    std::vector<pud_lodestar_choice> non_conjecture_ring() const;
    std::vector<pud_lodestar_choice> conjecture_ring() const;
    bool unfold(const pud_lodestar_choice& choice);
    bool propagate_unit();

    IManifest& manifest_;
    pud_lodestar_frontier& frontier_;
    size_t ratio_;
    size_t last_refuted_;
    bool last_unfold_on_focus_;
    pud_lodestar_schedule schedule_;
};

template<typename IManifest>
pud_lodestar_driver<IManifest>::pud_lodestar_driver(
    IManifest& manifest,
    pud_lodestar_frontier& frontier,
    size_t non_conjecture_unfolds_per_conjecture_unfold)
    : manifest_(manifest)
    , frontier_(frontier)
    , ratio_(non_conjecture_unfolds_per_conjecture_unfold)
    , last_refuted_(0)
    , last_unfold_on_focus_(false)
    , schedule_() {}

template<typename IManifest>
bool pud_lodestar_driver<IManifest>::on_focus(pud_global_node_id node) const {
    return manifest_.focus_set_.contains(node);
}

template<typename IManifest>
std::vector<typename pud_lodestar_driver<IManifest>::query_ref>
pud_lodestar_driver<IManifest>::conjecture_queries() const {
    std::vector<pud_global_node_id> nodes(frontier_.live().begin(), frontier_.live().end());
    std::vector<query_ref> queries;
    for (const pud_global_node_id node : nodes) {
        if (!on_focus(node))
            continue;
        if (manifest_.modules_.check_refuted(node))
            continue;
        const pud_active_state& state = manifest_.active_states_.get(node);
        std::vector<pud_body_goal_id> goals;
        for (const auto& [goal, expr] : manifest_.node_goals_.get(node).goals)
            goals.push_back(goal);
        std::sort(goals.begin(), goals.end());
        for (const pud_body_goal_id goal : goals) {
            bool found = false;
            pud_mhq_head_id head = 0;
            for (const auto& [stored_goal, stored_head] : state.queries) {
                if (stored_goal != goal)
                    continue;
                found = true;
                head = stored_head;
                break;
            }
            const bool ready = found && !manifest_.mhq_.candidates(head).empty();
            queries.push_back(query_ref{.node = node, .goal = goal, .head = head, .has_candidates = ready});
        }
    }
    return queries;
}

template<typename IManifest>
std::optional<pud_body_goal_id> pud_lodestar_driver<IManifest>::smallest_ready_goal(pud_global_node_id leaf) const {
    if (!frontier_.live().contains(leaf))
        return std::nullopt;
    if (manifest_.modules_.check_refuted(leaf))
        return std::nullopt;
    const pud_active_state& state = manifest_.active_states_.get(leaf);
    std::optional<pud_body_goal_id> smallest;
    for (const auto& [goal, head] : state.queries) {
        if (manifest_.mhq_.candidates(head).empty())
            continue;
        if (smallest.has_value() && goal >= smallest.value())
            continue;
        smallest = goal;
    }
    return smallest;
}

template<typename IManifest>
void pud_lodestar_driver<IManifest>::push_witness_slot(
    std::vector<pud_lodestar_choice>& ring,
    const query_ref& query,
    pud_mhcs_head_id candidate,
    uint8_t side,
    pud_global_node_id leaf) const {
    pud_lodestar_choice choice{
        .slot = pud_lodestar_slot_id{
            .query_node = query.node,
            .query_goal = query.goal,
            .candidate = candidate,
            .side = side,
        },
        .leaf = leaf,
        .body_goal = 0,
        .eligible = false,
    };
    const std::optional<pud_body_goal_id> ready = smallest_ready_goal(leaf);
    if (ready.has_value() && manifest_.unfold_allower_.allow(leaf)) {
        choice.body_goal = ready.value();
        choice.eligible = true;
    }
    ring.push_back(choice);
}

template<typename IManifest>
std::vector<pud_lodestar_choice> pud_lodestar_driver<IManifest>::non_conjecture_ring() const {
    std::vector<pud_lodestar_choice> ring;
    for (const query_ref& query : conjecture_queries()) {
        if (!query.has_candidates)
            continue;
        for (const pud_mhcs_head_id candidate : manifest_.mhq_.candidates(query.head)) {
            const auto* watched = manifest_.candidate_watch_.find(candidate);
            if (watched == nullptr)
                continue;
            if (watched->self_witness) {
                if (on_focus(watched->self_leaf))
                    continue;
                push_witness_slot(ring, query, candidate, k_lodestar_side_self, watched->self_leaf);
                continue;
            }
            uint8_t side = k_lodestar_side_a;
            for (const pud_mhws_head_id witness : watched->witnesses) {
                const std::optional<pud_global_node_id> leaf = manifest_.witness_log_.node_of(witness);
                if (leaf.has_value() && !on_focus(leaf.value()))
                    push_witness_slot(ring, query, candidate, side, leaf.value());
                side = k_lodestar_side_b;
            }
        }
    }
    std::sort(ring.begin(), ring.end(), [](const pud_lodestar_choice& left, const pud_lodestar_choice& right) {
        return left.slot < right.slot;
    });
    return ring;
}

template<typename IManifest>
std::vector<pud_lodestar_choice> pud_lodestar_driver<IManifest>::conjecture_ring() const {
    std::vector<pud_lodestar_choice> ring;
    for (const query_ref& query : conjecture_queries()) {
        if (!query.has_candidates)
            continue;
        for (const pud_mhcs_head_id candidate : manifest_.mhq_.candidates(query.head)) {
            const auto* watched = manifest_.candidate_watch_.find(candidate);
            if (watched == nullptr)
                continue;
            if (watched->self_witness) {
                if (!on_focus(watched->self_leaf))
                    continue;
                push_witness_slot(ring, query, candidate, k_lodestar_side_self, watched->self_leaf);
                continue;
            }
            uint8_t side = k_lodestar_side_a;
            for (const pud_mhws_head_id witness : watched->witnesses) {
                const std::optional<pud_global_node_id> leaf = manifest_.witness_log_.node_of(witness);
                if (leaf.has_value() && on_focus(leaf.value()))
                    push_witness_slot(ring, query, candidate, side, leaf.value());
                side = k_lodestar_side_b;
            }
        }
        ring.push_back(pud_lodestar_choice{
            .slot = pud_lodestar_slot_id{
                .query_node = query.node,
                .query_goal = query.goal,
                .candidate = std::numeric_limits<pud_mhcs_head_id>::max(),
                .side = k_lodestar_side_caller,
            },
            .leaf = query.node,
            .body_goal = query.goal,
            .eligible = manifest_.unfold_allower_.allow(query.node),
        });
    }
    std::sort(ring.begin(), ring.end(), [](const pud_lodestar_choice& left, const pud_lodestar_choice& right) {
        return left.slot < right.slot;
    });
    return ring;
}

template<typename IManifest>
bool pud_lodestar_driver<IManifest>::unfold(const pud_lodestar_choice& choice) {
    if (!manifest_.unfold_allower_.allow(choice.leaf)) {
        last_refuted_ = 0;
        last_unfold_on_focus_ = false;
        return false;
    }
    const pud_frontier_change change = manifest_.unfolder_.unfold(choice.leaf, choice.body_goal);
    frontier_.apply_unfold(choice.leaf, change);
    manifest_.focus_extender_.extend(choice.leaf, change.offspring);
    last_refuted_ = change.refuted.size();
    last_unfold_on_focus_ = on_focus(choice.leaf);
    if (last_unfold_on_focus_)
        schedule_.reset_ratio();
    else
        schedule_.count_non_conjecture();
    return true;
}

template<typename IManifest>
bool pud_lodestar_driver<IManifest>::propagate_unit() {
    while (!manifest_.unit_queries_.empty()) {
        const pud_mhq_head_id head = manifest_.unit_queries_.front();
        manifest_.unit_queries_.pop_front();
        const std::optional<pud_unfold> held = manifest_.query_holders_.find_holder(head);
        if (!held.has_value())
            continue;
        if (manifest_.mhq_.candidates(head).size() != 1)
            continue;
        std::vector<pud_global_node_id> ready;
        for (const pud_global_node_id leaf : manifest_.query_holders_.get_holders(head)) {
            if (!frontier_.live().contains(leaf))
                continue;
            if (manifest_.modules_.check_refuted(leaf))
                continue;
            if (!manifest_.unfold_allower_.allow(leaf))
                continue;
            ready.push_back(leaf);
        }
        if (ready.empty())
            continue;
        const pud_lodestar_choice choice{
            .slot = pud_lodestar_slot_id{
                .query_node = ready.front(),
                .query_goal = held->goal,
                .candidate = 0,
                .side = k_lodestar_side_self,
            },
            .leaf = ready.front(),
            .body_goal = held->goal,
            .eligible = true,
        };
        if (!unfold(choice))
            continue;
        if (ready.size() > 1)
            manifest_.unit_queries_.push_front(head);
        return true;
    }
    return false;
}

template<typename IManifest>
bool pud_lodestar_driver<IManifest>::refute_blocked() {
    last_refuted_ = 0;
    for (const query_ref& query : conjecture_queries()) {
        if (query.has_candidates)
            continue;
        const std::vector<pud_global_node_id> refuted = manifest_.leaf_refuter_.refute(query.node);
        if (refuted.empty())
            frontier_.erase_refuted({query.node});
        else
            frontier_.erase_refuted(refuted);
        last_refuted_ = refuted.empty() ? 1 : refuted.size();
        return true;
    }
    return false;
}

template<typename IManifest>
bool pud_lodestar_driver<IManifest>::unfold_one() {
    if (propagate_unit())
        return true;
    const bool below_ratio = schedule_.non_conjecture_since_caller() < ratio_;
    if (below_ratio) {
        const std::optional<pud_lodestar_choice> outside = schedule_.next_non_conjecture(non_conjecture_ring());
        if (outside.has_value() && unfold(outside.value()))
            return true;
    }
    const std::optional<pud_lodestar_choice> inside = schedule_.next_conjecture(conjecture_ring());
    if (inside.has_value() && unfold(inside.value()))
        return true;
    const std::optional<pud_lodestar_choice> more = schedule_.next_non_conjecture(non_conjecture_ring());
    if (!more.has_value())
        return false;
    return unfold(more.value());
}

template<typename IManifest>
size_t pud_lodestar_driver<IManifest>::last_refuted() const {
    return last_refuted_;
}

template<typename IManifest>
bool pud_lodestar_driver<IManifest>::last_unfold_on_focus() const {
    return last_unfold_on_focus_;
}

#endif
