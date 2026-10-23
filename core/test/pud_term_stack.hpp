#ifndef PUD_TERM_STACK_HPP
#define PUD_TERM_STACK_HPP

#include <cstdint>
#include <utility>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

// The real term machinery over one environment that starts empty: a bind map with the cycle
// detector, unifier and normalizer made for it.
struct pud_term_stack {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;

    expr_pool pool;
    globalizer globalize;
    term_factory_t terms{globalize, pool, pool};
    ubs_bind_map bind_map{pud_ubs_state{}};
    term_factory_t::cycle_detector_t cycle_detector{terms.make_cycle_detector(bind_map)};
    term_factory_t::unifier_t unifier{terms.make_unifier(bind_map, cycle_detector)};
    term_factory_t::normalizer_t normalizer{terms.make_normalizer(bind_map)};

    const expr* resolve(uint32_t var) {
        return normalizer.normalize(framed_expr{pool.make_var(var), 0});
    }

    void equate(uint32_t var_a, uint32_t var_b) {
        bind_map.equate(bind_map.find(var_a), bind_map.find(var_b));
    }

    void instantiate(uint32_t var, framed_expr value) {
        bind_map.instantiate(bind_map.find(var), value);
    }

    // The environment so far; editing continues on top of it.
    pud_ubs_state snapshot() {
        pud_ubs_state state = std::move(bind_map).persist();
        bind_map = ubs_bind_map{state};
        return state;
    }
};

inline framed_expr pud_fe(const expr* skeleton, uint32_t frame_offset) {
    return framed_expr{skeleton, frame_offset};
}

#endif
