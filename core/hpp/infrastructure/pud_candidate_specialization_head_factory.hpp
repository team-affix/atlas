#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_HPP

#include <utility>
#include "infrastructure/pud_candidate_specialization_head.hpp"

template<
    typename Descent,
    typename ChildIterator,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename ITryForkWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IPropagateDescent>
struct pud_candidate_specialization_head_factory {
    using head_type = pud_candidate_specialization_head<
        Descent,
        ChildIterator,
        ITryAddHead,
        IAdvanceWitnessSearchHead,
        ITryForkWitnessSearchHead,
        ICheckNodeLeaf,
        IGetChildren,
        IPropagateDescent>;

    pud_candidate_specialization_head_factory(
        ITryAddHead& try_add_head,
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        ITryForkWitnessSearchHead& try_fork_witness_search_head,
        ICheckNodeLeaf& check_node_leaf,
        IGetChildren& get_children,
        IPropagateDescent& descend);
    head_type make(Descent search_root_descent) const;
private:
    ITryAddHead& try_add_head_;
    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    ITryForkWitnessSearchHead& try_fork_witness_search_head_;
    ICheckNodeLeaf& check_node_leaf_;
    IGetChildren& get_children_;
    IPropagateDescent& descend_;
};

template<
    typename D,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename ITFWSH,
    typename ICNL,
    typename IGC,
    typename IDESC>
pud_candidate_specialization_head_factory<D, CI, ITAH, IAWSH, ITFWSH, ICNL, IGC, IDESC>::pud_candidate_specialization_head_factory(
    ITAH& try_add_head,
    IAWSH& advance_witness_search_head,
    ITFWSH& try_fork_witness_search_head,
    ICNL& check_node_leaf,
    IGC& get_children,
    IDESC& descend)
    : try_add_head_(try_add_head)
    , advance_witness_search_head_(advance_witness_search_head)
    , try_fork_witness_search_head_(try_fork_witness_search_head)
    , check_node_leaf_(check_node_leaf)
    , get_children_(get_children)
    , descend_(descend) {}

template<
    typename D,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename ITFWSH,
    typename ICNL,
    typename IGC,
    typename IDESC>
typename pud_candidate_specialization_head_factory<D, CI, ITAH, IAWSH, ITFWSH, ICNL, IGC, IDESC>::head_type
pud_candidate_specialization_head_factory<D, CI, ITAH, IAWSH, ITFWSH, ICNL, IGC, IDESC>::make(D search_root_descent) const {
    return head_type{
        try_add_head_,
        advance_witness_search_head_,
        try_fork_witness_search_head_,
        check_node_leaf_,
        get_children_,
        descend_,
        std::move(search_root_descent)};
}

#endif
