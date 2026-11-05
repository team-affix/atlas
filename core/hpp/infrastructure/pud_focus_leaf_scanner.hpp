#ifndef PUD_FOCUS_LEAF_SCANNER_HPP
#define PUD_FOCUS_LEAF_SCANNER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Activates a registered module when node states are worked out lazily: nothing is replayed. Only the focus module
// has offspring, the unrefuted leaves of the focus axiom's stored tree, found by walking ids, children and refuted
// flags alone. Leaves of other trees need no frontier entry; they are reached through witness searches. A large tree
// is walked with the caches trimmed every so many nodes, as no search step ends meanwhile to trim them.
template<typename IGetChildren, typename IIsNodeRefuted, typename IIsNodeLeaf, typename ITrimCaches>
struct pud_focus_leaf_scanner {
    pud_focus_leaf_scanner(
        IGetChildren& get_children,
        IIsNodeRefuted& is_node_refuted,
        IIsNodeLeaf& is_node_leaf,
        ITrimCaches& trim_caches,
        pud_global_node_id focus_axiom);
    pud_frontier_change activate_module(pud_module_id module) const;
private:
    static constexpr size_t k_nodes_between_trims = 4096;

    IGetChildren& get_children_;
    IIsNodeRefuted& is_node_refuted_;
    IIsNodeLeaf& is_node_leaf_;
    ITrimCaches& trim_caches_;
    pud_global_node_id focus_axiom_;
};

#define PUD_FOCUS_LEAF_SCANNER_TEMPLATE                                                                  \
    template<typename IGetChildren, typename IIsNodeRefuted, typename IIsNodeLeaf, typename ITrimCaches>
#define PUD_FOCUS_LEAF_SCANNER pud_focus_leaf_scanner<IGetChildren, IIsNodeRefuted, IIsNodeLeaf, ITrimCaches>

PUD_FOCUS_LEAF_SCANNER_TEMPLATE
PUD_FOCUS_LEAF_SCANNER::pud_focus_leaf_scanner(
    IGetChildren& get_children,
    IIsNodeRefuted& is_node_refuted,
    IIsNodeLeaf& is_node_leaf,
    ITrimCaches& trim_caches,
    pud_global_node_id focus_axiom)
    : get_children_(get_children)
    , is_node_refuted_(is_node_refuted)
    , is_node_leaf_(is_node_leaf)
    , trim_caches_(trim_caches)
    , focus_axiom_(focus_axiom) {}

PUD_FOCUS_LEAF_SCANNER_TEMPLATE
pud_frontier_change PUD_FOCUS_LEAF_SCANNER::activate_module(pud_module_id module) const {
    pud_frontier_change change{.offspring = {}, .refuted = {}};
    if (module != focus_axiom_.module)
        return change;
    std::vector<pud_global_node_id> pending{focus_axiom_};
    for (size_t visited = 1; !pending.empty(); ++visited) {
        if (visited % k_nodes_between_trims == 0)
            trim_caches_.trim();
        const pud_global_node_id node = pending.back();
        pending.pop_back();
        if (is_node_refuted_.is_refuted(node))
            continue;
        if (is_node_leaf_.is_leaf(node)) {
            change.offspring.push_back(node);
            continue;
        }
        for (const pud_node_id child : get_children_.get_children(node))
            pending.push_back(pud_global_node_id{.module = module, .node = child});
    }
    return change;
}

#undef PUD_FOCUS_LEAF_SCANNER
#undef PUD_FOCUS_LEAF_SCANNER_TEMPLATE

#endif
