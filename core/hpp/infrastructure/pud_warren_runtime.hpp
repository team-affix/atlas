#ifndef PUD_WARREN_RUNTIME_HPP
#define PUD_WARREN_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_warren_manifest.hpp"
#include "infrastructure/pud_paged_module_assembly.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_focus_leaf_counts.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_search_step.hpp"
#include "value_objects/pud_search_stop_reason.hpp"

struct pud_warren_runtime {
    pud_warren_runtime(
        expr_pool& exprs,
        pud_global_node_id focus_axiom,
        size_t max_unfolds,
        size_t cascade_depth,
        size_t max_node_depth,
        size_t max_open_goals,
        size_t state_cache_bytes,
        size_t trail_cache_bytes,
        size_t max_disk_bytes,
        bool report_stored_solutions);
    void register_module(pud_paged_module_assembly& module_files);
    bool next();
    pud_search_step last_step() const;
    const expr* solution() const;
    size_t solution_depth() const;
    pud_search_stop_reason stop_reason() const;
    size_t unfold_count() const;
    size_t unfold_depth_sum() const;
    size_t count_nodes() const;
    size_t state_cache_hits() const;
    size_t state_cache_misses() const;
    size_t disk_bytes() const;
    pud_focus_leaf_counts count_focus_leaves() const;
    bool has_pending_solution() const;
    const std::string& functor_name(uint32_t global_functor_id) const;
private:
    pud_warren_manifest manifest_;
};

#endif
