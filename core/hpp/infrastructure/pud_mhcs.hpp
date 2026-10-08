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
    typename QueryHandle,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
struct pud_mhcs {
    pud_mhcs(IMakeHead& make_head, IForkHead& fork_head);
    std::optional<pud_mhcs_head_id> try_add_head(QueryHandle search_root_handle);
    void remove_head(pud_mhcs_head_id head_id);
    std::vector<pud_mhcs_head_id> invalidate_leaf(pud_node_id node);
    std::optional<pud_mhcs_head_id> witness_refuted(pud_mhws_head_id witness_head_id);
    std::optional<pud_mhcs_head_id> try_fork_head(pud_mhcs_head_id head_id, QueryHandle new_query_handle);
private:
    IMakeHead& make_head_;
    IForkHead& fork_head_;

    void link(pud_mhcs_head_id head_id, pud_candidate_justification justification);
    pud_candidate_justification unlink_head(pud_mhcs_head_id head_id);
    std::unordered_set<pud_mhcs_head_id> unlink_self_witnesses(pud_node_id node);

    pud_mhcs_head_id next_head_id_;
    
    std::unordered_map<pud_mhcs_head_id, Head> heads_;
    std::unordered_map<pud_mhcs_head_id, QueryHandle> head_to_query_handle_;

    std::unordered_map<pud_mhcs_head_id, pud_candidate_justification> head_to_justification_;
    std::unordered_map<pud_mhws_head_id, pud_mhcs_head_id> witness_head_to_head_;
    std::unordered_map<pud_node_id, std::unordered_set<pud_mhcs_head_id>> leaf_to_heads_;
};

template<
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
pud_mhcs<QH, Head, IMakeHead, IForkHead>::pud_mhcs(IMakeHead& make_head, IForkHead& fork_head)
    : make_head_(make_head)
    , fork_head_(fork_head)
    , next_head_id_(0) {}

template<
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_mhcs_head_id> pud_mhcs<QH, Head, IMakeHead, IForkHead>::try_add_head(QH search_root_handle) {
    auto [head_it, head_inserted] = heads_.emplace(
        next_head_id_,
        make_head_.make(std::move(search_root_handle)));

    DEBUG_ASSERT(head_inserted);

    Head& head = head_it->second;

    std::optional<pud_candidate_resume_context<QH>> resume_context = head.resume();

    if (!resume_context.has_value()) {
        heads_.erase(head_it);
        return std::nullopt;
    }

    const pud_candidate_resume_context<QH>& context = resume_context.value();

    head_to_query_handle_.insert({next_head_id_, context.query_handle});

    link(next_head_id_, context.justification);

    return next_head_id_++;
}

template<
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
void pud_mhcs<QH, Head, IMakeHead, IForkHead>::remove_head(pud_mhcs_head_id head_id) {
    if (!heads_.contains(head_id))
        return;

    heads_.erase(head_id);
    head_to_query_handle_.erase(head_id);
    unlink_head(head_id);
}

template<
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::vector<pud_mhcs_head_id> pud_mhcs<QH, Head, IMakeHead, IForkHead>::invalidate_leaf(pud_node_id node) {
    auto head_ids = unlink_self_witnesses(node);

    std::vector<pud_mhcs_head_id> result;

    for (pud_mhcs_head_id head_id : head_ids) {
        Head& head = heads_.at(head_id);
        std::optional<pud_candidate_resume_context<QH>> resume_context = head.resume();

        if (!resume_context.has_value()) {
            heads_.erase(head_id);
            head_to_query_handle_.erase(head_id);
            result.push_back(head_id);
            continue;
        }

        const pud_candidate_resume_context<QH>& context = resume_context.value();
        head_to_query_handle_.at(head_id) = context.query_handle;
        link(head_id, context.justification);
    }

    return std::move(result);
}

template<
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_mhcs_head_id> pud_mhcs<QH, Head, IMakeHead, IForkHead>::witness_refuted(pud_mhws_head_id witness_head_id) {
    if (!witness_head_to_head_.contains(witness_head_id))
        return std::nullopt;

    pud_mhcs_head_id head_id = witness_head_to_head_.at(witness_head_id);

    unlink_head(head_id);

    Head& head = heads_.at(head_id);
    head.witness_refuted(witness_head_id);

    std::optional<pud_candidate_resume_context<QH>> resume_context = head.resume();

    if (!resume_context.has_value()) {
        heads_.erase(head_id);
        head_to_query_handle_.erase(head_id);
        return head_id;
    }

    const pud_candidate_resume_context<QH>& context = resume_context.value();
    head_to_query_handle_.at(head_id) = context.query_handle;
    link(head_id, context.justification);

    return std::nullopt;
}

template<
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_mhcs_head_id> pud_mhcs<QH, Head, IMakeHead, IForkHead>::try_fork_head(pud_mhcs_head_id head_id, QH new_query_handle) {
    Head& old_head = heads_.at(head_id);

    auto [new_head_it, new_head_inserted] = heads_.emplace(
        next_head_id_,
        fork_head_.fork(old_head, new_query_handle));

    DEBUG_ASSERT(new_head_inserted);

    Head& new_head = new_head_it->second;

    std::optional<pud_candidate_resume_context<QH>> resume_context = new_head.resume();

    if (!resume_context.has_value()) {
        heads_.erase(new_head_it);
        return std::nullopt;
    }

    const pud_candidate_resume_context<QH>& context = resume_context.value();

    head_to_query_handle_.insert({next_head_id_, context.query_handle});
    link(next_head_id_, context.justification);

    return next_head_id_++;
}

template<
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
void pud_mhcs<QH, Head, IMakeHead, IForkHead>::link(pud_mhcs_head_id head_id, pud_candidate_justification justification) {
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
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
pud_candidate_justification pud_mhcs<QH, Head, IMakeHead, IForkHead>::unlink_head(pud_mhcs_head_id head_id) {
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
    typename QH,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::unordered_set<pud_mhcs_head_id> pud_mhcs<QH, Head, IMakeHead, IForkHead>::unlink_self_witnesses(pud_node_id node) {
    auto extracted = leaf_to_heads_.extract(node);

    for (pud_mhcs_head_id head_id : extracted.mapped())
        head_to_justification_.erase(head_id);

    return std::move(extracted.mapped());
}

#endif
