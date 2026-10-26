#ifndef PUD_TERM_STACK_HPP
#define PUD_TERM_STACK_HPP

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_exporter_factory.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

// The real term machinery over one environment that starts empty: a bind map with the cycle
// detector, unifier and normalizer made for it.
struct pud_term_stack {
    using cycle_detector_factory_t = pud_cycle_detector_factory<globalizer, expr_pool>;
    using unifier_factory_t = pud_unifier_factory<globalizer, expr_pool>;
    using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, expr_pool>;
    using exporter_factory_t = pud_exporter_factory<globalizer, expr_pool, expr_pool>;

    expr_pool pool;
    globalizer globalize;
    pud_bind_map_factory bind_maps;
    cycle_detector_factory_t cycle_detectors{globalize, pool};
    unifier_factory_t unifiers{globalize, pool};
    normalizer_factory_t normalizers{globalize, pool, pool};
    exporter_factory_t exporters{globalize, pool, pool};
    ubs_bind_map bind_map{pud_ubs_state{}};
    cycle_detector_factory_t::cycle_detector_t cycle_detector{cycle_detectors.make_cycle_detector(bind_map)};
    unifier_factory_t::unifier_t unifier{unifiers.make_unifier(bind_map, cycle_detector)};
    normalizer_factory_t::normalizer_t normalizer{normalizers.make_normalizer(bind_map)};
};

#endif
