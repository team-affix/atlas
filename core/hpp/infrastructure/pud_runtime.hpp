#ifndef PUD_RUNTIME_HPP
#define PUD_RUNTIME_HPP

#include <cstdint>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_open_goal.hpp"

struct pud_runtime {
    pud_runtime(db& database);
    pud_frontier_change open_database();
    pud_frontier_change add_conjecture(const expr* head, const initial_goal_exprs& goals, uint32_t var_count);
    pud_frontier_change unfold(pud_derivation_id derivation, pud_body_goal_id goal);
    coroutine<pud_open_goal, void> iterate_body_goals(pud_derivation_id derivation);
    const expr* head_expr(pud_derivation_id derivation);
    const expr* body_goal_expr(pud_derivation_id derivation, pud_body_goal_id goal);
private:
    void clear_recorders();
    pud_frontier_change collect_change(pud_node_id first_new_node, const std::vector<pud_node_id>& produced);
    pud_node_id find_root(pud_node_id node);
    const expr* normalize_at_leaf(pud_node_id leaf, const expr* skeleton);

    db& database_;
    bool database_opened_;
    pud_manifest manifest_;
};

#endif
