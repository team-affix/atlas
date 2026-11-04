#include "infrastructure/pud_forager_runtime.hpp"

pud_forager_runtime::pud_forager_runtime(
    expr_pool& exprs,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t cascade_depth,
    size_t max_node_depth,
    size_t max_open_goals,
    bool report_stored_solutions)
    : manifest_(
          exprs, focus_axiom, max_unfolds, cascade_depth, max_node_depth,
          max_open_goals, report_stored_solutions) {}

void pud_forager_runtime::register_module(pud_module& module) {
    manifest_.module_loader_.load(module);
}

bool pud_forager_runtime::next() {
    return manifest_.search_driver_.next();
}

pud_search_step pud_forager_runtime::last_step() const {
    return manifest_.search_driver_.last_step();
}

const expr* pud_forager_runtime::solution() const {
    return manifest_.solution_store_.solution();
}

size_t pud_forager_runtime::solution_depth() const {
    return manifest_.solution_store_.solution_depth();
}

pud_search_stop_reason pud_forager_runtime::stop_reason() const {
    return manifest_.searcher_.stop_reason();
}

size_t pud_forager_runtime::unfold_count() const {
    return manifest_.searcher_.unfold_count();
}

size_t pud_forager_runtime::unfold_depth_sum() const {
    return manifest_.unfold_depth_recorder_.depth_sum();
}

size_t pud_forager_runtime::count_nodes() const {
    return manifest_.modules_.count_nodes();
}

pud_focus_leaf_counts pud_forager_runtime::count_focus_leaves() const {
    return manifest_.focus_leaf_tally_.count();
}

bool pud_forager_runtime::has_pending_solution() const {
    return manifest_.solved_leaf_finder_.find().has_value();
}

const std::string& pud_forager_runtime::functor_name(uint32_t global_functor_id) const {
    return manifest_.symbol_table_.get_name_by_global_functor_id(global_functor_id);
}
