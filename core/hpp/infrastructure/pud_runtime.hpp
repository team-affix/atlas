#ifndef PUD_RUNTIME_HPP
#define PUD_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "infrastructure/pud_whnf.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_frontier_change.hpp"

struct pud_runtime {
    pud_runtime(db& database);
    pud_frontier_change open_database();
    pud_frontier_change add_conjecture(const expr* head, const initial_goal_exprs& goals, uint32_t var_count);
    pud_frontier_change expand(pud_derivation_id derivation, pud_body_goal_id goal);
    const expr* head(pud_derivation_id derivation);
    std::vector<pud_body_goal> body_goals(pud_derivation_id derivation);
    size_t candidate_count(pud_derivation_id derivation, pud_body_goal_id goal);
    pud_whnf<globalizer, expr_pool, expr_pool> whnf(pud_derivation_id derivation);
private:
    pud_frontier_change collect_change(const std::vector<pud_node_id>& produced);
    pud_node_id find_root(pud_node_id node);

    db& database_;
    bool database_opened_;
    pud_manifest manifest_;
};

#endif
