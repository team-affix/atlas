#include "infrastructure/pud_lodestar_runtime.hpp"

pud_lodestar_runtime::pud_lodestar_runtime(
    expr_pool& exprs,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold,
    size_t max_node_depth)
    : manifest_(exprs, focus_axiom, max_unfolds, non_conjecture_unfolds_per_conjecture_unfold, max_node_depth) {}

void pud_lodestar_runtime::register_module(pud_module& module) {
    manifest_.module_registrar_.register_module(module);
}

bool pud_lodestar_runtime::next() {
    return manifest_.search_stepper_.step();
}

const expr* pud_lodestar_runtime::solution() const {
    return manifest_.solution_store_.solution();
}

size_t pud_lodestar_runtime::solution_depth() const {
    return manifest_.solution_store_.solution_depth();
}

const std::string& pud_lodestar_runtime::functor_name(uint32_t global_functor_id) const {
    return manifest_.symbol_table_.get_name_by_global_functor_id(global_functor_id);
}
