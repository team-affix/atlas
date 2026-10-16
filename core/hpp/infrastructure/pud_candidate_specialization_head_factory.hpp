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
    typename ICheckNodeRefuted,
    typename IGetChildren>
struct pud_candidate_specialization_head_factory {
    using head_type = pud_candidate_specialization_head<
        Descent,
        ChildIterator,
        ITryAddHead,
        IAdvanceWitnessSearchHead,
        ITryForkWitnessSearchHead,
        ICheckNodeLeaf,
        ICheckNodeRefuted,
        IGetChildren>;

    pud_candidate_specialization_head_factory(
        ITryAddHead& try_add_head,
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        ITryForkWitnessSearchHead& try_fork_witness_search_head,
        ICheckNodeLeaf& check_node_leaf,
        ICheckNodeRefuted& check_node_refuted,
        IGetChildren& get_children);
    head_type make(Descent search_root_descent) const;
private:
    ITryAddHead& try_add_head_;
    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    ITryForkWitnessSearchHead& try_fork_witness_search_head_;
    ICheckNodeLeaf& check_node_leaf_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetChildren& get_children_;
};

template<
    typename D,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename ITFWSH,
    typename ICNL,
    typename ICNR,
    typename IGC>
pud_candidate_specialization_head_factory<D, CI, ITAH, IAWSH, ITFWSH, ICNL, ICNR, IGC>::pud_candidate_specialization_head_factory(
    ITAH& try_add_head,
    IAWSH& advance_witness_search_head,
    ITFWSH& try_fork_witness_search_head,
    ICNL& check_node_leaf,
    ICNR& check_node_refuted,
    IGC& get_children)
    : try_add_head_(try_add_head)
    , advance_witness_search_head_(advance_witness_search_head)
    , try_fork_witness_search_head_(try_fork_witness_search_head)
    , check_node_leaf_(check_node_leaf)
    , check_node_refuted_(check_node_refuted)
    , get_children_(get_children) {}

template<
    typename D,
    typename CI,
    typename ITAH,
    typename IAWSH,
    typename ITFWSH,
    typename ICNL,
    typename ICNR,
    typename IGC>
typename pud_candidate_specialization_head_factory<D, CI, ITAH, IAWSH, ITFWSH, ICNL, ICNR, IGC>::head_type
pud_candidate_specialization_head_factory<D, CI, ITAH, IAWSH, ITFWSH, ICNL, ICNR, IGC>::make(D search_root_descent) const {
    return head_type{
        try_add_head_,
        advance_witness_search_head_,
        try_fork_witness_search_head_,
        check_node_leaf_,
        check_node_refuted_,
        get_children_,
        std::move(search_root_descent)};
}

#endif
