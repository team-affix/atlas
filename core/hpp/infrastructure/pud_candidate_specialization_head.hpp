#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_candidate_resume_context.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_candidate_choice_point.hpp"
#include "value_objects/pud_candidate_self_witness.hpp"
#include "value_objects/pud_witness_advance_result.hpp"
#include "debug_assert.hpp"

// Specializes a candidate as deep as its witnesses allow. A candidate at a leaf is its own witness;
// otherwise it keeps two witness searches down distinct children (a choice point), and when only one
// survives it advances toward it. Witness searches are reactive: a refuted one is reported through
// witness_refuted, and a held witness is always valid. A head that fails to resume releases every
// witness search it still holds.
template<
    typename Descent,
    typename NodeIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename ITryForkWitnessSearchHead,
    typename IRemoveWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename ICheckNodeRefuted,
    typename IGetChildren>
struct pud_candidate_specialization_head {
    pud_candidate_specialization_head(
        ITryAddHead& try_add_head,
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        ITryForkWitnessSearchHead& try_fork_witness_search_head,
        IRemoveWitnessSearchHead& remove_witness_search_head,
        ICheckNodeLeaf& check_node_leaf,
        ICheckNodeRefuted& check_node_refuted,
        IGetChildren& get_children,
        Descent specialization_root_descent);
    pud_candidate_specialization_head(
        const pud_candidate_specialization_head& other,
        Descent specialization_root_descent);
    std::optional<pud_candidate_resume_context<Descent>> resume();
    std::vector<pud_node_id> path() const;
    void witness_refuted(pud_mhws_head_id witness_id);
private:
    struct witness {
        pud_mhws_head_id id;
        Descent descent;
    };
    struct witness_scan {
        std::optional<witness> witness_a;
        std::optional<witness> witness_b;
        NodeIterator next_witness_root_it;
        NodeIterator end_witness_root_it;
    };

    std::optional<witness> try_fork_witness(const std::optional<witness>& other_witness);
    std::optional<witness> try_replace_witness();
    void advance(witness& survivor);
    void release_witnesses();

    ITryAddHead& try_add_head_;
    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    ITryForkWitnessSearchHead& try_fork_witness_search_head_;
    IRemoveWitnessSearchHead& remove_witness_search_head_;
    ICheckNodeLeaf& check_node_leaf_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetChildren& get_children_;

    pud_node_id root_;
    std::vector<pud_node_id> node_path_;
    Descent current_descent_;
    std::optional<witness_scan> witness_scan_;
    bool node_path_truncated_;
};

#define PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE                                         \
    template<typename Descent, typename NodeIterator, typename ITryAddHead,                \
             typename IAdvanceWitnessSearchHead, typename ITryForkWitnessSearchHead,       \
             typename IRemoveWitnessSearchHead, typename ICheckNodeLeaf,                   \
             typename ICheckNodeRefuted, typename IGetChildren>
#define PUD_CANDIDATE_SPECIALIZATION_HEAD                                                  \
    pud_candidate_specialization_head<Descent, NodeIterator, ITryAddHead,                  \
        IAdvanceWitnessSearchHead, ITryForkWitnessSearchHead, IRemoveWitnessSearchHead,    \
        ICheckNodeLeaf, ICheckNodeRefuted, IGetChildren>

PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
PUD_CANDIDATE_SPECIALIZATION_HEAD::pud_candidate_specialization_head(
    ITryAddHead& try_add_head,
    IAdvanceWitnessSearchHead& advance_witness_search_head,
    ITryForkWitnessSearchHead& try_fork_witness_search_head,
    IRemoveWitnessSearchHead& remove_witness_search_head,
    ICheckNodeLeaf& check_node_leaf,
    ICheckNodeRefuted& check_node_refuted,
    IGetChildren& get_children,
    Descent specialization_root_descent)
    : try_add_head_(try_add_head)
    , advance_witness_search_head_(advance_witness_search_head)
    , try_fork_witness_search_head_(try_fork_witness_search_head)
    , remove_witness_search_head_(remove_witness_search_head)
    , check_node_leaf_(check_node_leaf)
    , check_node_refuted_(check_node_refuted)
    , get_children_(get_children)
    , root_(specialization_root_descent.node())
    , node_path_()
    , current_descent_(specialization_root_descent)
    , witness_scan_()
    , node_path_truncated_(false) {}

// Replays the other head's path from the new root; a path that no longer replays truncates the
// head. A replayed choice point forks both of its witness searches toward the new root.
PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
PUD_CANDIDATE_SPECIALIZATION_HEAD::pud_candidate_specialization_head(
    const pud_candidate_specialization_head& other,
    Descent specialization_root_descent)
    : try_add_head_(other.try_add_head_)
    , advance_witness_search_head_(other.advance_witness_search_head_)
    , try_fork_witness_search_head_(other.try_fork_witness_search_head_)
    , remove_witness_search_head_(other.remove_witness_search_head_)
    , check_node_leaf_(other.check_node_leaf_)
    , check_node_refuted_(other.check_node_refuted_)
    , get_children_(other.get_children_)
    , root_(specialization_root_descent.node())
    , node_path_()
    , current_descent_(specialization_root_descent)
    , witness_scan_()
    , node_path_truncated_(false) {
    for (pud_node_id node : other.node_path_) {
        std::optional<Descent> next_descent = current_descent_.descend(node);
        if (!next_descent.has_value())
            break;
        node_path_.push_back(node);
        current_descent_ = next_descent.value();
    }

    const bool path_replayed = node_path_.size() == other.node_path_.size();
    if (!path_replayed) {
        node_path_truncated_ = true;
        return;
    }
    if (!other.witness_scan_.has_value())
        return;

    witness_scan_ = witness_scan{
        .witness_a = try_fork_witness(other.witness_scan_->witness_a),
        .witness_b = try_fork_witness(other.witness_scan_->witness_b),
        .next_witness_root_it = other.witness_scan_->next_witness_root_it,
        .end_witness_root_it = other.witness_scan_->end_witness_root_it,
    };
}

PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
std::optional<pud_candidate_resume_context<Descent>> PUD_CANDIDATE_SPECIALIZATION_HEAD::resume() {
    if (node_path_truncated_)
        return std::nullopt;

    while (true) {
        const pud_node_id current_node = current_descent_.node();

        if (check_node_refuted_.check_refuted(current_node)) {
            release_witnesses();
            return std::nullopt;
        }

        if (check_node_leaf_.check_leaf(current_node)) {
            witness_scan_ = std::nullopt;
            return pud_candidate_resume_context<Descent>{
                .justification = pud_candidate_self_witness{.node = current_node},
                .descent = current_descent_,
            };
        }

        if (!witness_scan_.has_value()) {
            const auto& children = get_children_.get(current_node);
            witness_scan_ = witness_scan{
                .witness_a = std::nullopt,
                .witness_b = std::nullopt,
                .next_witness_root_it = children.begin(),
                .end_witness_root_it = children.end(),
            };
        }

        std::optional<witness>& witness_a = witness_scan_->witness_a;
        std::optional<witness>& witness_b = witness_scan_->witness_b;
        if (!witness_a.has_value())
            witness_a = try_replace_witness();
        if (!witness_b.has_value())
            witness_b = try_replace_witness();

        if (witness_a.has_value() && witness_b.has_value())
            return pud_candidate_resume_context<Descent>{
                .justification = pud_candidate_choice_point{
                    .witness_a = witness_a.value().id,
                    .witness_b = witness_b.value().id,
                },
                .descent = current_descent_,
            };

        if (!witness_a.has_value() && !witness_b.has_value())
            return std::nullopt;

        advance(witness_a.has_value() ? witness_a.value() : witness_b.value());
    }
}

// Root first, the candidate's current node last.
PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
std::vector<pud_node_id> PUD_CANDIDATE_SPECIALIZATION_HEAD::path() const {
    std::vector<pud_node_id> path{root_};
    path.insert(path.end(), node_path_.begin(), node_path_.end());
    return path;
}

PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
void PUD_CANDIDATE_SPECIALIZATION_HEAD::witness_refuted(pud_mhws_head_id witness_id) {
    DEBUG_ASSERT(witness_scan_.has_value());
    std::optional<witness>& witness_a = witness_scan_->witness_a;
    std::optional<witness>& witness_b = witness_scan_->witness_b;
    if (witness_a.has_value() && witness_a.value().id == witness_id) {
        witness_a = std::nullopt;
        return;
    }
    DEBUG_ASSERT(witness_b.has_value() && witness_b.value().id == witness_id);
    witness_b = std::nullopt;
}

PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
std::optional<typename PUD_CANDIDATE_SPECIALIZATION_HEAD::witness>
PUD_CANDIDATE_SPECIALIZATION_HEAD::try_fork_witness(const std::optional<witness>& other_witness) {
    if (!other_witness.has_value())
        return std::nullopt;
    std::optional<Descent> search_root_descent = current_descent_.descend(other_witness->descent.node());
    if (!search_root_descent.has_value())
        return std::nullopt;
    std::optional<pud_mhws_head_id> forked_id =
        try_fork_witness_search_head_.try_fork_head(other_witness->id, search_root_descent.value());
    if (!forked_id.has_value())
        return std::nullopt;
    return witness{.id = forked_id.value(), .descent = search_root_descent.value()};
}

PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
std::optional<typename PUD_CANDIDATE_SPECIALIZATION_HEAD::witness>
PUD_CANDIDATE_SPECIALIZATION_HEAD::try_replace_witness() {
    NodeIterator& next_witness_root_it = witness_scan_->next_witness_root_it;
    const NodeIterator end_witness_root_it = witness_scan_->end_witness_root_it;
    while (next_witness_root_it != end_witness_root_it) {
        std::optional<Descent> search_root_descent = current_descent_.descend(*next_witness_root_it++);
        if (!search_root_descent.has_value())
            continue;
        std::optional<pud_mhws_head_id> added_id = try_add_head_.try_add_head(search_root_descent.value());
        if (!added_id.has_value())
            continue;
        return witness{.id = added_id.value(), .descent = search_root_descent.value()};
    }
    return std::nullopt;
}

// Steps onto the survivor's search root; the survivor's search moves one level down with it.
PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
void PUD_CANDIDATE_SPECIALIZATION_HEAD::advance(witness& survivor) {
    std::optional<pud_witness_advance_result<Descent, NodeIterator>> advanced =
        advance_witness_search_head_.advance_head(survivor.id);
    node_path_.push_back(survivor.descent.node());
    current_descent_ = survivor.descent;
    if (!advanced.has_value()) {
        witness_scan_ = std::nullopt;
        return;
    }
    survivor.descent = advanced->root_descent;
    witness_scan_->next_witness_root_it = advanced->root_next_sibling_it;
    witness_scan_->end_witness_root_it = advanced->root_end_sibling_it;
}

PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE
void PUD_CANDIDATE_SPECIALIZATION_HEAD::release_witnesses() {
    if (!witness_scan_.has_value())
        return;
    if (witness_scan_->witness_a.has_value())
        remove_witness_search_head_.remove_head(witness_scan_->witness_a->id);
    if (witness_scan_->witness_b.has_value())
        remove_witness_search_head_.remove_head(witness_scan_->witness_b->id);
    witness_scan_ = std::nullopt;
}

#undef PUD_CANDIDATE_SPECIALIZATION_HEAD
#undef PUD_CANDIDATE_SPECIALIZATION_HEAD_TEMPLATE

#endif
