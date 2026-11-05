#ifndef PUD_AXIOM_FOCUS_TEST_HPP
#define PUD_AXIOM_FOCUS_TEST_HPP

#include <optional>
#include "value_objects/pud_global_node_id.hpp"

// Tells whether a node lies under the focus: whether the root its parent links lead to is the focus axiom.
template<typename IGetParent>
struct pud_axiom_focus_test {
    pud_axiom_focus_test(IGetParent& get_parent, pud_global_node_id focus_axiom);
    bool contains(pud_global_node_id node) const;
private:
    IGetParent& get_parent_;
    pud_global_node_id focus_axiom_;
};

template<typename IGetParent>
pud_axiom_focus_test<IGetParent>::pud_axiom_focus_test(IGetParent& get_parent, pud_global_node_id focus_axiom)
    : get_parent_(get_parent)
    , focus_axiom_(focus_axiom) {}

template<typename IGetParent>
bool pud_axiom_focus_test<IGetParent>::contains(pud_global_node_id node) const {
    if (node.module != focus_axiom_.module)
        return false;
    pud_global_node_id root = node;
    for (std::optional<pud_global_node_id> parent = get_parent_.get_parent(root); parent.has_value();
         parent = get_parent_.get_parent(root))
        root = parent.value();
    return root == focus_axiom_;
}

#endif
