#ifndef PUD_REFUTED_LEAF_RECORDER_HPP
#define PUD_REFUTED_LEAF_RECORDER_HPP

#include <utility>
#include <vector>
#include "value_objects/pud_node_id.hpp"

template<typename IMarkRefuted, typename ICheckNodeLeaf>
struct pud_refuted_leaf_recorder {
    pud_refuted_leaf_recorder(IMarkRefuted& mark_refuted, ICheckNodeLeaf& check_node_leaf);
    void mark_refuted(pud_node_id node);
    std::vector<pud_node_id> take_refuted_leaves();
private:
    IMarkRefuted& mark_refuted_;
    ICheckNodeLeaf& check_node_leaf_;
    std::vector<pud_node_id> refuted_leaves_;
};

#define PUD_REFUTED_LEAF_RECORDER_TEMPLATE template<typename IMarkRefuted, typename ICheckNodeLeaf>
#define PUD_REFUTED_LEAF_RECORDER pud_refuted_leaf_recorder<IMarkRefuted, ICheckNodeLeaf>

PUD_REFUTED_LEAF_RECORDER_TEMPLATE
PUD_REFUTED_LEAF_RECORDER::pud_refuted_leaf_recorder(IMarkRefuted& mark_refuted, ICheckNodeLeaf& check_node_leaf)
    : mark_refuted_(mark_refuted)
    , check_node_leaf_(check_node_leaf)
    , refuted_leaves_() {}

PUD_REFUTED_LEAF_RECORDER_TEMPLATE
void PUD_REFUTED_LEAF_RECORDER::mark_refuted(pud_node_id node) {
    mark_refuted_.mark_refuted(node);
    if (!check_node_leaf_.check_leaf(node))
        return;
    refuted_leaves_.push_back(node);
}

PUD_REFUTED_LEAF_RECORDER_TEMPLATE
std::vector<pud_node_id> PUD_REFUTED_LEAF_RECORDER::take_refuted_leaves() {
    return std::exchange(refuted_leaves_, {});
}

#undef PUD_REFUTED_LEAF_RECORDER
#undef PUD_REFUTED_LEAF_RECORDER_TEMPLATE

#endif
