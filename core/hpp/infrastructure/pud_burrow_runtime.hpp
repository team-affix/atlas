#ifndef PUD_BURROW_RUNTIME_HPP
#define PUD_BURROW_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_burrow_manifest.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_focus_leaf_counts.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_search_step.hpp"
#include "value_objects/pud_search_stop_reason.hpp"

struct pud_burrow_runtime {
    pud_burrow_runtime(
        expr_pool& exprs,
        pud_global_node_id focus_axiom,
        size_t max_unfolds,
        size_t cascade_depth,
        size_t max_node_depth,
        size_t max_open_goals,
        size_t state_cache_bytes,
        bool report_stored_solutions);
    void register_module(pud_module& module);
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
    pud_focus_leaf_counts count_focus_leaves() const;
    bool has_pending_solution() const;
    const std::string& functor_name(uint32_t global_functor_id) const;
private:
    pud_burrow_manifest manifest_;
};

#endif
