#ifndef PUD_TERM_STACK_HPP
#define PUD_TERM_STACK_HPP

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
};

#endif
