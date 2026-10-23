#ifndef PUD_ROOT_OPENER_HPP
#define PUD_ROOT_OPENER_HPP

#include <cstdint>
#include <set>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"

// Adds roots to the database: axioms, which queries search, and conjectures, which are only solved.
// Roots are added first and opened together, so every query of a batch sees every axiom of it.
template<
    typename IMakeRoot,
    typename IAddAxiom,
    typename IActivateRoot,
    typename IRefuteLeaf,
    typename IReleaseLeaf,
    typename IInvalidateQueryLeaf,
    typename IGetQueryHolders,
    typename ICollectChange>
struct pud_root_opener {
    pud_root_opener(
        IMakeRoot& make_root,
        IAddAxiom& add_axiom,
        IActivateRoot& activate_root,
        IRefuteLeaf& refute_leaf,
        IReleaseLeaf& release_leaf,
        IInvalidateQueryLeaf& invalidate_query_leaf,
        IGetQueryHolders& get_query_holders,
        ICollectChange& collect_change);
    pud_node_id add_axiom(const rule& axiom);
    pud_node_id add_conjecture(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count);
    pud_frontier_change open();
private:
    IMakeRoot& make_root_;
    IAddAxiom& add_axiom_;
    IActivateRoot& activate_root_;
    IRefuteLeaf& refute_leaf_;
    IReleaseLeaf& release_leaf_;
    IInvalidateQueryLeaf& invalidate_query_leaf_;
    IGetQueryHolders& get_query_holders_;
    ICollectChange& collect_change_;
    std::vector<pud_node_id> pending_roots_;
};

#define PUD_ROOT_OPENER_TEMPLATE                                                           \
    template<typename IMakeRoot, typename IAddAxiom, typename IActivateRoot, typename IRefuteLeaf, \
             typename IReleaseLeaf, typename IInvalidateQueryLeaf, typename IGetQueryHolders, typename ICollectChange>
#define PUD_ROOT_OPENER                                                                    \
    pud_root_opener<IMakeRoot, IAddAxiom, IActivateRoot, IRefuteLeaf, IReleaseLeaf,            \
        IInvalidateQueryLeaf, IGetQueryHolders, ICollectChange>

PUD_ROOT_OPENER_TEMPLATE
PUD_ROOT_OPENER::pud_root_opener(
    IMakeRoot& make_root,
    IAddAxiom& add_axiom,
    IActivateRoot& activate_root,
    IRefuteLeaf& refute_leaf,
    IReleaseLeaf& release_leaf,
    IInvalidateQueryLeaf& invalidate_query_leaf,
    IGetQueryHolders& get_query_holders,
    ICollectChange& collect_change)
    : make_root_(make_root)
    , add_axiom_(add_axiom)
    , activate_root_(activate_root)
    , refute_leaf_(refute_leaf)
    , release_leaf_(release_leaf)
    , invalidate_query_leaf_(invalidate_query_leaf)
    , get_query_holders_(get_query_holders)
    , collect_change_(collect_change)
    , pending_roots_() {}

PUD_ROOT_OPENER_TEMPLATE
pud_node_id PUD_ROOT_OPENER::add_axiom(const rule& axiom) {
    const pud_node_id root = make_root_.make_root(axiom.head, axiom.body, axiom.var_count);
    add_axiom_.push_back(root);
    pending_roots_.push_back(root);
    return root;
}

PUD_ROOT_OPENER_TEMPLATE
pud_node_id PUD_ROOT_OPENER::add_conjecture(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count) {
    const pud_node_id root = make_root_.make_root(head, goals, var_count);
    pending_roots_.push_back(root);
    return root;
}

PUD_ROOT_OPENER_TEMPLATE
pud_frontier_change PUD_ROOT_OPENER::open() {
    const std::vector<pud_node_id> roots = std::move(pending_roots_);
    pending_roots_.clear();
    std::vector<pud_node_id> refuted_roots;
    for (pud_node_id root : roots) {
        if (!activate_root_.activate_root(root))
            continue;
        refuted_roots.push_back(root);
    }
    std::vector<pud_node_id> all_refuted;
    std::vector<pud_node_id> releasing;
    std::vector<pud_node_id> refuting = refuted_roots;
    while (!releasing.empty() || !refuting.empty()) {
        for (pud_node_id node : refuting) {
            refute_leaf_.refute(node);
            all_refuted.push_back(node);
            releasing.push_back(node);
        }
        refuting.clear();
        const std::vector<pud_node_id> releasable = std::move(releasing);
        releasing.clear();
        for (pud_node_id node : releasable)
            release_leaf_.release(node);
        std::set<pud_node_id> next_refuting;
        for (pud_node_id node : releasable) {
            for (pud_mhq_head_id head_id : invalidate_query_leaf_.invalidate_leaf(node)) {
                const std::vector<pud_node_id> held_by = get_query_holders_.get_holders(head_id);
                next_refuting.insert(held_by.begin(), held_by.end());
            }
        }
        refuting.assign(next_refuting.begin(), next_refuting.end());
    }
    for (pud_node_id root : roots)
        collect_change_.add_produced(root);
    for (pud_node_id refuted : all_refuted)
        collect_change_.add_refuted(refuted);
    return collect_change_.finish();
}

#undef PUD_ROOT_OPENER
#undef PUD_ROOT_OPENER_TEMPLATE

#endif
