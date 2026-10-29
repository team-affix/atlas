#ifndef PUD_LODESTAR_RUNTIME_HPP
#define PUD_LODESTAR_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_lodestar_manifest.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_module_id.hpp"

struct pud_lodestar_runtime {
    pud_lodestar_runtime(
        expr_pool& exprs,
        pud_module_id entry_module,
        size_t max_unfolds,
        size_t non_conjecture_unfolds_per_conjecture_unfold);
    void register_module(pud_module& module);
    bool next();
    const expr* solution() const;
    size_t solution_depth() const;
private:
    pud_lodestar_manifest manifest_;
};

#endif
