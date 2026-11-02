#ifndef PUD_TERM_STACK_HPP
#define PUD_TERM_STACK_HPP

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_exporter_factory.hpp"
#include "pud_identity_demoduler.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

// The real term machinery over one environment that starts empty: a bind map with the cycle
// detector, unifier and normalizer made for it.
struct pud_term_stack {
    using copy_demoduler_t = pud_copy_functor_demoduler<expr_pool>;
    using cycle_detector_factory_t = pud_cycle_detector_factory<globalizer, copy_demoduler_t>;
    using unifier_factory_t = pud_unifier_factory<globalizer, expr_pool, pud_identity_global_functor_ids, cycle_detector_factory_t::cycle_detector_t>;
    using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, expr_pool, copy_demoduler_t>;
    using exporter_factory_t = pud_exporter_factory<globalizer, expr_pool, expr_pool, pud_identity_functor_relinker>;

    expr_pool pool;
    globalizer globalize;
    copy_demoduler_t copy_demoduler{pool};
    pud_identity_global_functor_ids global_functor_ids;
    pud_identity_functor_relinker functor_relinker;
    pud_bind_map_factory bind_maps;
    cycle_detector_factory_t cycle_detectors{globalize, copy_demoduler};
    unifier_factory_t unifiers{globalize, pool, global_functor_ids};
    normalizer_factory_t normalizers{globalize, pool, pool, copy_demoduler};
    exporter_factory_t exporters{globalize, pool, pool, functor_relinker};
    ubs_bind_map bind_map{pud_ubs_state{}};
    cycle_detector_factory_t::cycle_detector_t cycle_detector{cycle_detectors.make_cycle_detector(bind_map)};
    unifier_factory_t::unifier_t unifier{unifiers.make_unifier(bind_map, cycle_detector)};
    normalizer_factory_t::normalizer_t normalizer{normalizers.make_normalizer(bind_map)};
};

#endif
