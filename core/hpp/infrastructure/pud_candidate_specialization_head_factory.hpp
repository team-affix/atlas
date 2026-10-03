#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_HPP

#include <utility>
#include "infrastructure/pud_candidate_specialization_head.hpp"

template<
    typename QueryHandle,
    typename ChildIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename IForkWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IPropagateQueryHandle>
struct pud_candidate_specialization_head_factory {
    using head_type = pud_candidate_specialization_head<
        QueryHandle,
        ChildIterator,
        ITryAddHead,
        IAdvanceWitnessSearchHead,
        IForkWitnessSearchHead,
        ICheckNodeLeaf,
        IGetChildren,
        IPropagateQueryHandle>;

    pud_candidate_specialization_head_factory(
        ITryAddHead& try_add_head,
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        IForkWitnessSearchHead& fork_witness_search_head,
        ICheckNodeLeaf& check_node_leaf,
        IGetChildren& get_children,
        IPropagateQueryHandle& propagate_query_handle);
    head_type make(QueryHandle search_root_handle) const;
private:
    ITryAddHead& try_add_head_;
    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    IForkWitnessSearchHead& fork_witness_search_head_;
    ICheckNodeLeaf& check_node_leaf_;
    IGetChildren& get_children_;
    IPropagateQueryHandle& propagate_query_handle_;
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
pud_candidate_specialization_head_factory<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::pud_candidate_specialization_head_factory(
    ITAH& try_add_head,
    IAWSH& advance_witness_search_head,
    IFWSH& fork_witness_search_head,
    ICNL& check_node_leaf,
    IGC& get_children,
    IPQH& propagate_query_handle)
    : try_add_head_(try_add_head)
    , advance_witness_search_head_(advance_witness_search_head)
    , fork_witness_search_head_(fork_witness_search_head)
    , check_node_leaf_(check_node_leaf)
    , get_children_(get_children)
    , propagate_query_handle_(propagate_query_handle) {}

template<
    typename QH,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename IFWSH,
    typename ICNL,
    typename IGC,
    typename IPQH>
typename pud_candidate_specialization_head_factory<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::head_type
pud_candidate_specialization_head_factory<QH, CI, ITAH, IAWSH, IFWSH, ICNL, IGC, IPQH>::make(QH search_root_handle) const {
    return head_type{
        try_add_head_,
        advance_witness_search_head_,
        fork_witness_search_head_,
        check_node_leaf_,
        get_children_,
        propagate_query_handle_,
        std::move(search_root_handle)};
}

#endif
