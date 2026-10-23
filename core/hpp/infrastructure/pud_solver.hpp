#ifndef PUD_SOLVER_HPP
#define PUD_SOLVER_HPP

#include <cstddef>
#include <initializer_list>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_solution.hpp"
#include "value_objects/pud_solver_step.hpp"
#include "value_objects/pud_unfold.hpp"

// Solves conjectures over the database; a conjecture is named by its root. Each step reports a solution if one is waiting, else performs
// an unfold a unit query forces, wherever it is, else expands a chosen open leaf of some conjecture.
// A forced unfold never adds leaves, so taking those first is free.
template<
    typename IAddConjecture,
    typename IOpenRoots,
    typename IExpand,
    typename IFindUnitUnfold,
    typename IChooseExpansion,
    typename IMaterialize>
struct pud_solver {
    pud_solver(
        IAddConjecture& add_conjecture,
        IOpenRoots& open_roots,
        IExpand& expand,
        IFindUnitUnfold& find_unit_unfold,
        IChooseExpansion& choose_expansion,
        IMaterialize& materialize);
    pud_node_id add_conjecture(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count);
    coroutine<pud_solver_step, void> solve();
    bool exhausted(pud_node_id conjecture) const;
private:
    void adopt(const pud_frontier_change& change, pud_node_id conjecture);
    void drop(const std::vector<pud_node_id>& refuted);
    void release(pud_node_id leaf);

    IAddConjecture& add_conjecture_;
    IOpenRoots& open_roots_;
    IExpand& expand_;
    IFindUnitUnfold& find_unit_unfold_;
    IChooseExpansion& choose_expansion_;
    IMaterialize& materialize_;
    std::map<pud_node_id, const expr*> conjecture_heads_;
    std::map<pud_node_id, size_t> conjecture_leaf_counts_;
    std::set<pud_node_id> frontier_;
    std::set<pud_node_id> unreported_;
    std::map<pud_node_id, pud_node_id> conjectures_;
};

#define PUD_SOLVER_TEMPLATE                                                                \
    template<typename IAddConjecture, typename IOpenRoots, typename IExpand,               \
             typename IFindUnitUnfold, typename IChooseExpansion, typename IMaterialize>
#define PUD_SOLVER                                                                         \
    pud_solver<IAddConjecture, IOpenRoots, IExpand, IFindUnitUnfold, IChooseExpansion, IMaterialize>

PUD_SOLVER_TEMPLATE
PUD_SOLVER::pud_solver(
    IAddConjecture& add_conjecture,
    IOpenRoots& open_roots,
    IExpand& expand,
    IFindUnitUnfold& find_unit_unfold,
    IChooseExpansion& choose_expansion,
    IMaterialize& materialize)
    : add_conjecture_(add_conjecture)
    , open_roots_(open_roots)
    , expand_(expand)
    , find_unit_unfold_(find_unit_unfold)
    , choose_expansion_(choose_expansion)
    , materialize_(materialize)
    , conjecture_heads_()
    , conjecture_leaf_counts_()
    , frontier_()
    , unreported_()
    , conjectures_() {}

PUD_SOLVER_TEMPLATE
pud_node_id PUD_SOLVER::add_conjecture(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count) {
    const pud_node_id conjecture = add_conjecture_.add_conjecture(head, goals, var_count);
    conjecture_heads_.emplace(conjecture, head);
    conjecture_leaf_counts_.emplace(conjecture, 0);
    const pud_frontier_change change = open_roots_.open();
    drop(change.refuted);
    adopt(change, conjecture);
    return conjecture;
}

// A forced unfold may lie in an axiom's tree; the leaves it opens there belong to no conjecture.
PUD_SOLVER_TEMPLATE
coroutine<pud_solver_step, void> PUD_SOLVER::solve() {
    while (true) {
        if (!unreported_.empty()) {
            const pud_node_id leaf = *unreported_.begin();
            const pud_node_id conjecture = conjectures_.at(leaf);
            release(leaf);
            co_yield pud_solution{
                .conjecture = conjecture,
                .leaf = leaf,
                .answer = materialize_.materialize(leaf, conjecture_heads_.at(conjecture)),
            };
            continue;
        }
        std::optional<pud_unfold> unfold = find_unit_unfold_.find_unit_unfold();
        if (!unfold.has_value() && frontier_.empty())
            co_return;
        if (!unfold.has_value())
            unfold = choose_expansion_.choose_expansion(frontier_);
        const auto owner = conjectures_.find(unfold->leaf);
        const std::optional<pud_node_id> conjecture =
            owner == conjectures_.end() ? std::nullopt : std::optional<pud_node_id>{owner->second};
        if (conjecture.has_value())
            release(unfold->leaf);
        const pud_frontier_change change = expand_.expand(unfold->leaf, unfold->goal);
        drop(change.refuted);
        if (conjecture.has_value())
            adopt(change, conjecture.value());
        co_yield unfold.value();
    }
}

PUD_SOLVER_TEMPLATE
bool PUD_SOLVER::exhausted(pud_node_id conjecture) const {
    return conjecture_leaf_counts_.at(conjecture) == 0;
}

PUD_SOLVER_TEMPLATE
void PUD_SOLVER::adopt(const pud_frontier_change& change, pud_node_id conjecture) {
    for (pud_node_id leaf : change.opened)
        frontier_.insert(leaf);
    for (pud_node_id leaf : change.proven)
        unreported_.insert(leaf);
    for (const std::vector<pud_node_id>* adopted : {&change.opened, &change.proven}) {
        for (pud_node_id leaf : *adopted)
            conjectures_.emplace(leaf, conjecture);
        conjecture_leaf_counts_.at(conjecture) += adopted->size();
    }
}

PUD_SOLVER_TEMPLATE
void PUD_SOLVER::drop(const std::vector<pud_node_id>& refuted) {
    for (pud_node_id leaf : refuted) {
        if (!conjectures_.contains(leaf))
            continue;
        release(leaf);
    }
}

// Stops tracking a conjecture leaf that is expanded, refuted or reported.
PUD_SOLVER_TEMPLATE
void PUD_SOLVER::release(pud_node_id leaf) {
    frontier_.erase(leaf);
    unreported_.erase(leaf);
    --conjecture_leaf_counts_.at(conjectures_.extract(leaf).mapped());
}

#undef PUD_SOLVER
#undef PUD_SOLVER_TEMPLATE

#endif
