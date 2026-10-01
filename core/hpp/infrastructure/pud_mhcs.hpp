#ifndef PUDC_MHCS_HPP
#define PUDC_MHCS_HPP

#include <optional>
#include <utility>
#include <variant>
#include <unordered_set>
#include <unordered_map>
#include "infrastructure/pud_candidate_search_head.hpp"
#include "value_objects/pud_candidate_justification.hpp"
#include "value_objects/pud_candidate_resume_context.hpp"
#include "value_objects/pud_query_position.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "debug_assert.hpp"

template<
    typename QueryHandle,
    typename ChildIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename IForkWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IPropagateQueryHandle>
struct pud_mhcs {
    std::optional<pud_mhcs_head_id> try_add_head(pud_query_position<QueryHandle> search_root_position);
    void remove_head(pud_mhcs_head_id head_id);
    std::vector<pud_mhcs_head_id> invalidate_leaf(const pud_node* node);
    void witness_refuted(pud_mhws_head_id witness_head_id);
    std::optional<pud_mhcs_head_id> try_fork_head(pud_mhcs_head_id head_id, QueryHandle new_query_handle);
private:
    using head_type = pud_candidate_search_head<
        QueryHandle,
        ChildIterator,
        ITryAddHead,
        IAdvanceWitnessSearchHead,
        IForkWitnessSearchHead,
        ICheckNodeLeaf,
        IGetChildren,
        IPropagateQueryHandle
    >;
    ITryAddHead& try_add_head_;
    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    IForkWitnessSearchHead& fork_witness_search_head_;
    ICheckNodeLeaf& check_node_leaf_;
    IGetChildren& get_children_;
    IPropagateQueryHandle& propagate_query_handle_;

    void link(pud_mhcs_head_id head_id, pud_candidate_justification justification);
    pud_candidate_justification unlink_head(pud_mhcs_head_id head_id);
    std::unordered_set<pud_mhcs_head_id> unlink_justification(pud_candidate_justification justification);

    pud_mhcs_head_id next_head_id_;
    
    std::unordered_map<pud_mhcs_head_id, head_type> heads_;
    std::unordered_map<pud_mhcs_head_id, QueryHandle> head_to_query_handle_;

    std::unordered_map<pud_mhcs_head_id, pud_candidate_justification> head_to_justification_;
    std::unordered_map<pud_mhws_head_id, pud_mhcs_head_id> witness_head_to_head_;
    std::unordered_map<const pud_node*, std::unordered_set<pud_mhcs_head_id>> leaf_to_heads_;
};

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
std::optional<pud_mhcs_head_id> pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::try_add_head(pud_query_position<QH> search_root_position) {
    auto [head_it, head_inserted] = heads_.emplace(
        next_head_id_,
        head_type{
            try_add_head_,
            advance_witness_search_head_,
            fork_witness_search_head_,
            check_node_leaf_,
            get_children_,
            propagate_query_handle_,
            std::move(search_root_position)});

    DEBUG_ASSERT(head_inserted);

    head_type& head = head_it->second;

    std::optional<pud_candidate_resume_context<QH>> resume_context = head.resume();

    if (!resume_context.has_value()) {
        heads_.erase(head_it);
        return std::nullopt;
    }

    const pud_candidate_resume_context<QH>& context = resume_context.value();

    head_to_query_handle_.insert({next_head_id_, context.query_handle});

    head_to_justification_.insert({next_head_id_, context.justification});

    link(next_head_id_, context.justification);

    return next_head_id_++;
}

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
void pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::remove_head(pud_mhcs_head_id head_id) {
}

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
std::vector<pud_mhcs_head_id> pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::invalidate_leaf(const pud_node* node) {
}

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
void pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::witness_refuted(pud_mhws_head_id witness_head_id) {
}

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
std::optional<pud_mhcs_head_id> pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::try_fork_head(pud_mhcs_head_id head_id, QH new_query_handle) {
}

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
void pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::link(pud_mhcs_head_id head_id, pud_candidate_justification justification) {
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
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
pud_candidate_justification pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::unlink_head(pud_mhcs_head_id head_id) {
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
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
std::unordered_set<pud_mhcs_head_id> pud_mhcs<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::unlink_justification(pud_candidate_justification justification) {
    if (auto choice_point = std::get_if<pud_candidate_choice_point>(&justification)) {
        pud_mhcs_head_id head_id = witness_head_to_head_.at(choice_point->witness_a);

        witness_head_to_head_.erase(choice_point->witness_a);
        witness_head_to_head_.erase(choice_point->witness_b);

        head_to_justification_.erase(head_id);

        return {head_id};
    }

    const pud_candidate_self_witness& self_witness = std::get<pud_candidate_self_witness>(justification);

    auto extracted = leaf_to_heads_.extract(self_witness.node);

    for (pud_mhcs_head_id head_id : extracted.mapped())
        head_to_justification_.erase(head_id);

    return std::move(extracted.mapped());
}

#endif
