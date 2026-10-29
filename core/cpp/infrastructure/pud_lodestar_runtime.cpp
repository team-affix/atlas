#include "infrastructure/pud_lodestar_runtime.hpp"

pud_lodestar_runtime::pud_lodestar_runtime(
    expr_pool& exprs,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold)
    : manifest_(exprs, focus_axiom, max_unfolds, non_conjecture_unfolds_per_conjecture_unfold) {}

void pud_lodestar_runtime::register_module(pud_module& module) {
    manifest_.register_module(module);
}

bool pud_lodestar_runtime::next() {
    return manifest_.next();
}

const expr* pud_lodestar_runtime::solution() const {
    return manifest_.solution();
}

size_t pud_lodestar_runtime::solution_depth() const {
    return manifest_.solution_depth();
}
