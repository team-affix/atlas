#ifndef PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP
#define PUD_WITNESS_SEARCH_HEAD_FACTORY_HPP

#include <utility>

// Makes witness search heads of the given type, which must be constructible from the leaf, refutation
// and children checks and a search root descent.
template<
    typename Head,
    typename Descent,
    typename ICheckNodeLeaf,
    typename ICheckNodeRefuted,
    typename IGetNodeChildren>
struct pud_witness_search_head_factory {
    pud_witness_search_head_factory(
        ICheckNodeLeaf& check_node_leaf,
        ICheckNodeRefuted& check_node_refuted,
        IGetNodeChildren& get_node_children);
    Head make(Descent search_root_descent) const;
private:
    ICheckNodeLeaf& check_node_leaf_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetNodeChildren& get_node_children_;
};

template<typename H, typename D, typename ICNL, typename ICNR, typename IGNC>
pud_witness_search_head_factory<H, D, ICNL, ICNR, IGNC>::pud_witness_search_head_factory(
    ICNL& check_node_leaf,
    ICNR& check_node_refuted,
    IGNC& get_node_children)
    : check_node_leaf_(check_node_leaf)
    , check_node_refuted_(check_node_refuted)
    , get_node_children_(get_node_children) {}

template<typename H, typename D, typename ICNL, typename ICNR, typename IGNC>
H pud_witness_search_head_factory<H, D, ICNL, ICNR, IGNC>::make(D search_root_descent) const {
    return H{check_node_leaf_, check_node_refuted_, get_node_children_, std::move(search_root_descent)};
}

#endif
