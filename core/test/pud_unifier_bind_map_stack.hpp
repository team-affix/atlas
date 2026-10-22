#ifndef PUD_UNIFIER_BIND_MAP_STACK_HPP
#define PUD_UNIFIER_BIND_MAP_STACK_HPP

#include <unordered_map>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_cycle_detector.hpp"
#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "ubs_bind_map_test_maps.hpp"
#include "value_objects/framed_expr.hpp"

struct ubs_bind_map_expr_pool_backing {
    expr_pool pool;
    ubs_std_parent_map parents;
    ubs_std_size_map sizes;
    ubs_std_rep_map reps;
    ubs_std_value_map values;

    using bind_map_t = ubs_bind_map<
        ubs_std_parent_map, ubs_std_parent_map, ubs_std_size_map, ubs_std_size_map,
        ubs_std_rep_map, ubs_std_rep_map, ubs_std_value_map, ubs_std_value_map>;

    bind_map_t make_bind_map() {
        return bind_map_t{parents, parents, sizes, sizes, reps, reps, values, values};
    }
};

struct pud_unifier_bind_map_stack {
    using bind_map_t = ubs_bind_map_expr_pool_backing::bind_map_t;
    using normalizer_t = global_normalizer<globalizer, expr_pool, expr_pool, bind_map_t, bind_map_t>;
    using cycle_detector_t = pud_cycle_detector<globalizer, bind_map_t, bind_map_t>;
    using pud_unifier_t = pud_unifier<globalizer, bind_map_t, bind_map_t, bind_map_t, bind_map_t, cycle_detector_t>;

    globalizer globalize;
    ubs_bind_map_expr_pool_backing bind_backing;
    bind_map_t bind_map{bind_backing.make_bind_map()};
    normalizer_t normalizer{globalize, bind_backing.pool, bind_backing.pool, bind_map, bind_map};
    cycle_detector_t cycle_detector{globalize, bind_map, bind_map};
    pud_unifier_t framed_unifier{globalize, bind_map, bind_map, bind_map, bind_map, cycle_detector};

    const expr* resolve(uint32_t var) {
        return normalizer.normalize(framed_expr{bind_backing.pool.make_var(var), 0});
    }
};

inline framed_expr pud_fe(const expr* skeleton, uint32_t frame_offset) {
    return framed_expr{skeleton, frame_offset};
}

inline ubs_map_snapshot snapshot_bind_backing(const ubs_bind_map_expr_pool_backing& backing) {
    return ubs_map_snapshot{backing.parents.map, backing.sizes.map, backing.reps.map, backing.values.map};
}

inline bool pud_expr_bindings_equal(const expr* left, const expr* right) {
    if (left == right)
        return true;
    if (left->content.index() != right->content.index())
        return false;
    if (const expr::var* left_var = std::get_if<expr::var>(&left->content)) {
        const expr::var* right_var = std::get_if<expr::var>(&right->content);
        return left_var->index == right_var->index;
    }
    const expr::functor& left_functor = std::get<expr::functor>(left->content);
    const expr::functor& right_functor = std::get<expr::functor>(right->content);
    if (left_functor.id != right_functor.id || left_functor.args.size() != right_functor.args.size())
        return false;
    for (size_t arg = 0; arg < left_functor.args.size(); ++arg) {
        if (!pud_expr_bindings_equal(left_functor.args[arg], right_functor.args[arg]))
            return false;
    }
    return true;
}

#endif
