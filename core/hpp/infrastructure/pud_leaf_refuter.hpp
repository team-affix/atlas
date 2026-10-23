#ifndef PUD_LEAF_REFUTER_HPP
#define PUD_LEAF_REFUTER_HPP

#include "value_objects/pud_node_id.hpp"

// Marks one leaf refuted and propagates that death up the tree.
template<typename IMarkLeafRefuted, typename IPropagateRefutation>
struct pud_leaf_refuter {
    pud_leaf_refuter(IMarkLeafRefuted& mark_leaf_refuted, IPropagateRefutation& propagate_refutation);
    void refute(pud_node_id leaf);
private:
    IMarkLeafRefuted& mark_leaf_refuted_;
    IPropagateRefutation& propagate_refutation_;
};

template<typename IMLR, typename IPR>
pud_leaf_refuter<IMLR, IPR>::pud_leaf_refuter(IMLR& mark_leaf_refuted, IPR& propagate_refutation)
    : mark_leaf_refuted_(mark_leaf_refuted)
    , propagate_refutation_(propagate_refutation) {}

template<typename IMLR, typename IPR>
void pud_leaf_refuter<IMLR, IPR>::refute(pud_node_id leaf) {
    mark_leaf_refuted_.mark_refuted(leaf);
    propagate_refutation_.refute(leaf);
}

#endif
