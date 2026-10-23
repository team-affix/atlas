#ifndef PUD_ROOT_OPENER_HPP
#define PUD_ROOT_OPENER_HPP

#include <cstdint>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"

// Adds roots to the database: axioms, which queries search, and conjectures, which are only solved.
// Roots are added first and opened together, so every query of a batch sees every axiom of it.
template<typename IMakeRoot, typename IAddAxiom, typename IActivateRoot, typename IRetire, typename ICollectChange>
struct pud_root_opener {
    pud_root_opener(
        IMakeRoot& make_root,
        IAddAxiom& add_axiom,
        IActivateRoot& activate_root,
        IRetire& retire,
        ICollectChange& collect_change);
    pud_node_id add_axiom(const rule& axiom);
    pud_node_id add_conjecture(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count);
    pud_frontier_change open();
private:
    IMakeRoot& make_root_;
    IAddAxiom& add_axiom_;
    IActivateRoot& activate_root_;
    IRetire& retire_;
    ICollectChange& collect_change_;
    std::vector<pud_node_id> pending_roots_;
};

#define PUD_ROOT_OPENER_TEMPLATE                                                           \
    template<typename IMakeRoot, typename IAddAxiom, typename IActivateRoot, typename IRetire, \
             typename ICollectChange>
#define PUD_ROOT_OPENER pud_root_opener<IMakeRoot, IAddAxiom, IActivateRoot, IRetire, ICollectChange>

PUD_ROOT_OPENER_TEMPLATE
PUD_ROOT_OPENER::pud_root_opener(
    IMakeRoot& make_root,
    IAddAxiom& add_axiom,
    IActivateRoot& activate_root,
    IRetire& retire,
    ICollectChange& collect_change)
    : make_root_(make_root)
    , add_axiom_(add_axiom)
    , activate_root_(activate_root)
    , retire_(retire)
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
    return collect_change_.collect(roots, retire_.retire({}, refuted_roots));
}

#undef PUD_ROOT_OPENER
#undef PUD_ROOT_OPENER_TEMPLATE

#endif
