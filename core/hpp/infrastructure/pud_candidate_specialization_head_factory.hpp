#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_HPP

#include <utility>

// Makes candidate specialization heads of the given type, which must be constructible from the witness
// search capabilities, the leaf, refutation and children checks, and a specialization root descent.
template<
    typename Head,
    typename Descent,
    typename ITryAddHead,
    typename IAdvanceWitnessSearchHead,
    typename ITryForkWitnessSearchHead,
    typename IRemoveWitnessSearchHead,
    typename ICheckNodeLeaf,
    typename ICheckNodeRefuted,
    typename IGetChildren>
struct pud_candidate_specialization_head_factory {
    pud_candidate_specialization_head_factory(
        ITryAddHead& try_add_head,
        IAdvanceWitnessSearchHead& advance_witness_search_head,
        ITryForkWitnessSearchHead& try_fork_witness_search_head,
        IRemoveWitnessSearchHead& remove_witness_search_head,
        ICheckNodeLeaf& check_node_leaf,
        ICheckNodeRefuted& check_node_refuted,
        IGetChildren& get_children);
    Head make(Descent search_root_descent) const;
private:
    ITryAddHead& try_add_head_;
    IAdvanceWitnessSearchHead& advance_witness_search_head_;
    ITryForkWitnessSearchHead& try_fork_witness_search_head_;
    IRemoveWitnessSearchHead& remove_witness_search_head_;
    ICheckNodeLeaf& check_node_leaf_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetChildren& get_children_;
};

#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_TEMPLATE                                 \
    template<typename Head, typename Descent, typename ITryAddHead,                        \
             typename IAdvanceWitnessSearchHead, typename ITryForkWitnessSearchHead,       \
             typename IRemoveWitnessSearchHead, typename ICheckNodeLeaf,                   \
             typename ICheckNodeRefuted, typename IGetChildren>
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY                                          \
    pud_candidate_specialization_head_factory<Head, Descent, ITryAddHead,                  \
        IAdvanceWitnessSearchHead, ITryForkWitnessSearchHead, IRemoveWitnessSearchHead,    \
        ICheckNodeLeaf, ICheckNodeRefuted, IGetChildren>

PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_TEMPLATE
PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY::pud_candidate_specialization_head_factory(
    ITryAddHead& try_add_head,
    IAdvanceWitnessSearchHead& advance_witness_search_head,
    ITryForkWitnessSearchHead& try_fork_witness_search_head,
    IRemoveWitnessSearchHead& remove_witness_search_head,
    ICheckNodeLeaf& check_node_leaf,
    ICheckNodeRefuted& check_node_refuted,
    IGetChildren& get_children)
    : try_add_head_(try_add_head)
    , advance_witness_search_head_(advance_witness_search_head)
    , try_fork_witness_search_head_(try_fork_witness_search_head)
    , remove_witness_search_head_(remove_witness_search_head)
    , check_node_leaf_(check_node_leaf)
    , check_node_refuted_(check_node_refuted)
    , get_children_(get_children) {}

PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_TEMPLATE
Head PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY::make(Descent search_root_descent) const {
    return Head{
        try_add_head_,
        advance_witness_search_head_,
        try_fork_witness_search_head_,
        remove_witness_search_head_,
        check_node_leaf_,
        check_node_refuted_,
        get_children_,
        std::move(search_root_descent)};
}

#undef PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY
#undef PUD_CANDIDATE_SPECIALIZATION_HEAD_FACTORY_TEMPLATE

#endif
