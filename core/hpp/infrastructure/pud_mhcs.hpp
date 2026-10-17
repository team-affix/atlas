#ifndef PUD_MHCS_HPP
#define PUD_MHCS_HPP

#include <optional>
#include <utility>
#include <variant>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "value_objects/pud_candidate_justification.hpp"
#include "value_objects/pud_candidate_resume_context.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

template<
    typename Descent,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
struct pud_mhcs {
    pud_mhcs(IMakeHead& make_head, IForkHead& fork_head, IRemoveWitnessHead& remove_witness_head);
    std::optional<pud_mhcs_head_id> try_add_head(Descent search_root_descent);
    void remove_head(pud_mhcs_head_id head_id);
    std::vector<pud_mhcs_head_id> invalidate_leaf(pud_node_id node);
    std::vector<pud_mhcs_head_id> witnesses_refuted(const std::vector<pud_mhws_head_id>& witness_head_ids);
    std::optional<pud_mhcs_head_id> try_fork_head(pud_mhcs_head_id head_id, Descent new_root_descent);
    std::vector<pud_node_id> get_path(pud_mhcs_head_id head_id) const;
    bool check_head(pud_mhcs_head_id head_id) const;
private:
    IMakeHead& make_head_;
    IForkHead& fork_head_;
    IRemoveWitnessHead& remove_witness_head_;

    bool try_justify(pud_mhcs_head_id head_id);
    void link(pud_mhcs_head_id head_id, pud_candidate_justification justification);
    pud_candidate_justification unlink_head(pud_mhcs_head_id head_id);
    std::unordered_set<pud_mhcs_head_id> unlink_self_witnesses(pud_node_id node);

    pud_mhcs_head_id next_head_id_;
    
    std::unordered_map<pud_mhcs_head_id, Head> heads_;

    std::unordered_map<pud_mhcs_head_id, pud_candidate_justification> head_to_justification_;
    std::unordered_map<pud_mhws_head_id, pud_mhcs_head_id> witness_head_to_head_;
    std::unordered_map<pud_node_id, std::unordered_set<pud_mhcs_head_id>> leaf_to_heads_;
};

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::pud_mhcs(IMakeHead& make_head, IForkHead& fork_head, IRemoveWitnessHead& remove_witness_head)
    : make_head_(make_head)
    , fork_head_(fork_head)
    , remove_witness_head_(remove_witness_head)
    , next_head_id_(0) {}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
std::optional<pud_mhcs_head_id> pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::try_add_head(D search_root_descent) {
    const bool head_inserted = heads_.emplace(next_head_id_, make_head_.make(std::move(search_root_descent))).second;
    DEBUG_ASSERT(head_inserted);
    if (!try_justify(next_head_id_))
        return std::nullopt;
    return next_head_id_++;
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
void pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::remove_head(pud_mhcs_head_id head_id) {
    if (!heads_.contains(head_id))
        return;

    heads_.erase(head_id);
    const pud_candidate_justification justification = unlink_head(head_id);
    const pud_candidate_choice_point* choice_point = std::get_if<pud_candidate_choice_point>(&justification);
    if (!choice_point)
        return;
    remove_witness_head_.remove_head(choice_point->witness_a);
    remove_witness_head_.remove_head(choice_point->witness_b);
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
std::vector<pud_mhcs_head_id> pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::invalidate_leaf(pud_node_id node) {
    std::vector<pud_mhcs_head_id> refuted;
    for (pud_mhcs_head_id head_id : unlink_self_witnesses(node)) {
        if (try_justify(head_id))
            continue;
        refuted.push_back(head_id);
    }
    return refuted;
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
std::vector<pud_mhcs_head_id> pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::witnesses_refuted(
    const std::vector<pud_mhws_head_id>& witness_head_ids) {
    std::vector<std::pair<pud_mhcs_head_id, pud_mhws_head_id>> refutations;
    for (pud_mhws_head_id witness_head_id : witness_head_ids) {
        auto witness_it = witness_head_to_head_.find(witness_head_id);
        if (witness_it == witness_head_to_head_.end())
            continue;
        refutations.emplace_back(witness_it->second, witness_head_id);
    }

    std::vector<pud_mhcs_head_id> affected;
    for (const auto& [head_id, witness_head_id] : refutations) {
        if (head_to_justification_.contains(head_id)) {
            unlink_head(head_id);
            affected.push_back(head_id);
        }
        heads_.at(head_id).witness_refuted(witness_head_id);
    }

    std::vector<pud_mhcs_head_id> refuted;
    for (pud_mhcs_head_id head_id : affected) {
        if (try_justify(head_id))
            continue;
        refuted.push_back(head_id);
    }
    return refuted;
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
std::optional<pud_mhcs_head_id> pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::try_fork_head(pud_mhcs_head_id head_id, D new_root_descent) {
    const bool head_inserted = heads_.emplace(next_head_id_, fork_head_.fork(heads_.at(head_id), new_root_descent)).second;
    DEBUG_ASSERT(head_inserted);
    if (!try_justify(next_head_id_))
        return std::nullopt;
    return next_head_id_++;
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
std::vector<pud_node_id> pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::get_path(pud_mhcs_head_id head_id) const {
    return heads_.at(head_id).path();
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
bool pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::check_head(pud_mhcs_head_id head_id) const {
    return heads_.contains(head_id);
}

// Resumes a head and links the justification it finds; a head that finds none is dropped.
template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
bool pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::try_justify(pud_mhcs_head_id head_id) {
    std::optional<pud_candidate_resume_context<D>> resume_context = heads_.at(head_id).resume();
    if (!resume_context.has_value()) {
        heads_.erase(head_id);
        return false;
    }
    link(head_id, resume_context->justification);
    return true;
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
void pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::link(pud_mhcs_head_id head_id, pud_candidate_justification justification) {
    head_to_justification_.insert({head_id, justification});

    if (auto choice_point = std::get_if<pud_candidate_choice_point>(&justification)) {
        witness_head_to_head_.insert({choice_point->witness_a, head_id});
        witness_head_to_head_.insert({choice_point->witness_b, head_id});
        return;
    }

    const pud_candidate_self_witness& self_witness = std::get<pud_candidate_self_witness>(justification);

    leaf_to_heads_[self_witness.node].insert(head_id);
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
pud_candidate_justification pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::unlink_head(pud_mhcs_head_id head_id) {
    auto extracted = head_to_justification_.extract(head_id);

    pud_candidate_justification justification = std::move(extracted.mapped());

    if (auto choice_point = std::get_if<pud_candidate_choice_point>(&justification)) {
        witness_head_to_head_.erase(choice_point->witness_a);
        witness_head_to_head_.erase(choice_point->witness_b);
        return justification;
    }

    const pud_candidate_self_witness& self_witness = std::get<pud_candidate_self_witness>(justification);

    auto& head_ids = leaf_to_heads_.at(self_witness.node);
    head_ids.erase(head_id);

    if (head_ids.empty())
        leaf_to_heads_.erase(self_witness.node);

    return justification;
}

template<
    typename D,
    typename Head,
    typename IMakeHead,
    typename IForkHead,
    typename IRemoveWitnessHead>
std::unordered_set<pud_mhcs_head_id> pud_mhcs<D, Head, IMakeHead, IForkHead, IRemoveWitnessHead>::unlink_self_witnesses(pud_node_id node) {
    auto extracted = leaf_to_heads_.extract(node);

    if (extracted.empty())
        return {};
    
    for (pud_mhcs_head_id head_id : extracted.mapped())
        head_to_justification_.erase(head_id);

    return std::move(extracted.mapped());
}

#endif
