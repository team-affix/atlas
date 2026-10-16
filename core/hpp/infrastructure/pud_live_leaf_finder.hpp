#ifndef PUD_LIVE_LEAF_FINDER_HPP
#define PUD_LIVE_LEAF_FINDER_HPP

#include <vector>
#include "value_objects/pud_node_id.hpp"

template<typename ICheckNodeRefuted, typename ICheckNodeLeaf, typename IGetNodeChildren>
struct pud_live_leaf_finder {
    pud_live_leaf_finder(
        ICheckNodeRefuted& check_node_refuted,
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children);
    std::vector<pud_node_id> find_live_leaves(pud_node_id root) const;
private:
    ICheckNodeRefuted& check_node_refuted_;
    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
};

#define PUD_LIVE_LEAF_FINDER_TEMPLATE                                                      \
    template<typename ICheckNodeRefuted, typename ICheckNodeLeaf, typename IGetNodeChildren>
#define PUD_LIVE_LEAF_FINDER pud_live_leaf_finder<ICheckNodeRefuted, ICheckNodeLeaf, IGetNodeChildren>

PUD_LIVE_LEAF_FINDER_TEMPLATE
PUD_LIVE_LEAF_FINDER::pud_live_leaf_finder(
    ICheckNodeRefuted& check_node_refuted,
    ICheckNodeLeaf& check_node_leaf,
    IGetNodeChildren& get_node_children)
    : check_node_refuted_(check_node_refuted)
    , check_node_leaf_(check_node_leaf)
    , get_node_children_(get_node_children) {}

PUD_LIVE_LEAF_FINDER_TEMPLATE
std::vector<pud_node_id> PUD_LIVE_LEAF_FINDER::find_live_leaves(pud_node_id root) const {
    std::vector<pud_node_id> live_leaves;
    std::vector<pud_node_id> pending{root};
    while (!pending.empty()) {
        const pud_node_id node = pending.back();
        pending.pop_back();
        if (check_node_refuted_.check_refuted(node))
            continue;
        if (check_node_leaf_.check_leaf(node)) {
            live_leaves.push_back(node);
            continue;
        }
        for (pud_node_id child : get_node_children_.get(node))
            pending.push_back(child);
    }
    return live_leaves;
}

#undef PUD_LIVE_LEAF_FINDER
#undef PUD_LIVE_LEAF_FINDER_TEMPLATE

#endif
