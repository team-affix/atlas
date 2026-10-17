#ifndef PUD_BASE_DESCENT_RUNNER_HPP
#define PUD_BASE_DESCENT_RUNNER_HPP

#include <utility>
#include <vector>
#include "value_objects/pud_node_id.hpp"

// Performs the base descent over the whole database: every axiom tree is descended from its root
// down to its live leaves, and each live leaf is activated with the descent that reached it.
// A node that arrives with a refuted goal is refuted along with its subtree.
template<
    typename BaseDescent,
    typename IGetAxioms,
    typename ICheckNodeRefuted,
    typename IMakeRootDescent,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IActivateLeaf,
    typename IRefuteNode>
struct pud_base_descent_runner {
    pud_base_descent_runner(
        IGetAxioms& get_axioms,
        ICheckNodeRefuted& check_node_refuted,
        IMakeRootDescent& make_root_descent,
        ICheckNodeLeaf& check_node_leaf,
        IGetChildren& get_children,
        IActivateLeaf& activate_leaf,
        IRefuteNode& refute_node);
    void run();
private:
    IGetAxioms& get_axioms_;
    ICheckNodeRefuted& check_node_refuted_;
    IMakeRootDescent& make_root_descent_;
    ICheckNodeLeaf& check_node_leaf_;
    IGetChildren& get_children_;
    IActivateLeaf& activate_leaf_;
    IRefuteNode& refute_node_;
};

#define PUD_BASE_DESCENT_RUNNER_TEMPLATE                                                   \
    template<typename BaseDescent, typename IGetAxioms, typename ICheckNodeRefuted,       \
             typename IMakeRootDescent, typename ICheckNodeLeaf, typename IGetChildren,     \
             typename IActivateLeaf, typename IRefuteNode>
#define PUD_BASE_DESCENT_RUNNER                                                            \
    pud_base_descent_runner<BaseDescent, IGetAxioms, ICheckNodeRefuted, IMakeRootDescent,  \
        ICheckNodeLeaf, IGetChildren, IActivateLeaf, IRefuteNode>

PUD_BASE_DESCENT_RUNNER_TEMPLATE
PUD_BASE_DESCENT_RUNNER::pud_base_descent_runner(
    IGetAxioms& get_axioms,
    ICheckNodeRefuted& check_node_refuted,
    IMakeRootDescent& make_root_descent,
    ICheckNodeLeaf& check_node_leaf,
    IGetChildren& get_children,
    IActivateLeaf& activate_leaf,
    IRefuteNode& refute_node)
    : get_axioms_(get_axioms)
    , check_node_refuted_(check_node_refuted)
    , make_root_descent_(make_root_descent)
    , check_node_leaf_(check_node_leaf)
    , get_children_(get_children)
    , activate_leaf_(activate_leaf)
    , refute_node_(refute_node) {}

PUD_BASE_DESCENT_RUNNER_TEMPLATE
void PUD_BASE_DESCENT_RUNNER::run() {
    std::vector<std::pair<pud_node_id, BaseDescent>> pending;
    for (pud_node_id axiom : get_axioms_.get()) {
        if (check_node_refuted_.check_refuted(axiom))
            continue;
        pending.emplace_back(axiom, make_root_descent_.make(axiom));
        while (!pending.empty()) {
            auto [node, descent] = std::move(pending.back());
            pending.pop_back();
            if (check_node_refuted_.check_refuted(node))
                continue;
            if (check_node_leaf_.check_leaf(node)) {
                activate_leaf_.activate(node, std::move(descent));
                continue;
            }
            const bool goal_refuted = !descent.body_goal_pq().refuted_goals().empty();
            if (goal_refuted) {
                refute_node_.refute(node);
                continue;
            }
            for (pud_node_id child : get_children_.get(node))
                pending.emplace_back(child, descent.descend(node, child));
        }
    }
}

#undef PUD_BASE_DESCENT_RUNNER
#undef PUD_BASE_DESCENT_RUNNER_TEMPLATE

#endif
