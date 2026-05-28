#ifndef WIRE_LOCATOR_HPP
#define WIRE_LOCATOR_HPP

#include "infrastructure/locator.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/overlay_bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/expr_pool.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_bind_map_factory.hpp"
#include "interfaces/i_overlay_bind_map_factory.hpp"
#include "interfaces/i_unifier_factory.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_import_expr.hpp"
#include "interfaces/i_get_expr_count.hpp"

inline void wire_mhu_deps(locator& loc,
    trail& t,
    bind_map& bm,
    bind_map_factory& bmf,
    overlay_bind_map_factory& obmf,
    unifier_factory& uf,
    lineage_pool& lp,
    goal_candidate_rules& ggcr,
    expr_pool& pool) {
    loc.bind_as<i_log_to_current_trail_frame>(t);
    loc.bind_as<i_bind_map>(bm);
    loc.bind_as<i_bind_map_factory>(bmf);
    loc.bind_as<i_overlay_bind_map_factory>(obmf);
    loc.bind_as<i_unifier_factory>(uf);
    loc.bind_as<i_make_resolution_lineage>(lp);
    loc.bind_as<i_get_goal_candidate_rule_ids>(ggcr);
    loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(pool);
}

#endif
