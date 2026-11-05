#include "infrastructure/pud_burrow_runtime.hpp"

pud_burrow_runtime::pud_burrow_runtime(
    expr_pool& exprs,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t cascade_depth,
    size_t max_node_depth,
    size_t max_open_goals,
    size_t state_cache_bytes,
    size_t max_disk_bytes,
    bool report_stored_solutions)
    : manifest_(
          exprs, focus_axiom, max_unfolds, cascade_depth, max_node_depth,
          max_open_goals, state_cache_bytes,
          max_disk_bytes, report_stored_solutions) {}

void pud_burrow_runtime::register_module(pud_paged_module_assembly& module_files) {
    manifest_.module_trim_list_.add(module_files.module());
    manifest_.disk_meter_.add(module_files);
    manifest_.module_loader_.load(module_files.module());
}

bool pud_burrow_runtime::next() {
    return manifest_.search_driver_.next();
}

pud_search_step pud_burrow_runtime::last_step() const {
    return manifest_.search_driver_.last_step();
}

const expr* pud_burrow_runtime::solution() const {
    return manifest_.solution_store_.solution();
}

size_t pud_burrow_runtime::solution_depth() const {
    return manifest_.solution_store_.solution_depth();
}

pud_search_stop_reason pud_burrow_runtime::stop_reason() const {
    if (manifest_.disk_budget_step_.exceeded())
        return pud_search_stop_reason::disk_full;
    return manifest_.searcher_.stop_reason();
}

size_t pud_burrow_runtime::unfold_count() const {
    return manifest_.searcher_.unfold_count();
}

size_t pud_burrow_runtime::unfold_depth_sum() const {
    return manifest_.unfold_depth_recorder_.depth_sum();
}

size_t pud_burrow_runtime::count_nodes() const {
    return manifest_.modules_.count_nodes();
}

size_t pud_burrow_runtime::disk_bytes() const {
    return manifest_.disk_meter_.bytes();
}

size_t pud_burrow_runtime::state_cache_hits() const {
    return manifest_.node_states_.hits();
}

size_t pud_burrow_runtime::state_cache_misses() const {
    return manifest_.node_states_.misses();
}

pud_focus_leaf_counts pud_burrow_runtime::count_focus_leaves() const {
    return manifest_.focus_leaf_tally_.count();
}

bool pud_burrow_runtime::has_pending_solution() const {
    return manifest_.solved_leaf_finder_.find().has_value();
}

const std::string& pud_burrow_runtime::functor_name(uint32_t global_functor_id) const {
    return manifest_.symbol_table_.get_name_by_global_functor_id(global_functor_id);
}
