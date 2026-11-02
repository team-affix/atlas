#ifndef PUD_SOLUTION_NORMALIZER_HPP
#define PUD_SOLUTION_NORMALIZER_HPP

#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Normalizes a module-local head under its leaf's bindings: the head is demoduled into global functor
// ids, then the kernel normalizer walks it (iteratively) through the bind map.
template<typename IDemodule, typename IMakeNormalizer>
struct pud_solution_normalizer {
    pud_solution_normalizer(IDemodule& demodule, IMakeNormalizer& make_normalizer);
    const expr* normalize(const expr* skeleton, pud_module_id module, const pud_ubs_state& bindings) const;
private:
    IDemodule& demodule_;
    IMakeNormalizer& make_normalizer_;
};

template<typename IDemodule, typename IMakeNormalizer>
pud_solution_normalizer<IDemodule, IMakeNormalizer>::pud_solution_normalizer(
    IDemodule& demodule, IMakeNormalizer& make_normalizer)
    : demodule_(demodule)
    , make_normalizer_(make_normalizer) {}

template<typename IDemodule, typename IMakeNormalizer>
const expr* pud_solution_normalizer<IDemodule, IMakeNormalizer>::normalize(
    const expr* skeleton, pud_module_id module, const pud_ubs_state& bindings) const {
    ubs_bind_map bind_map{bindings};
    const expr* global_skeleton = demodule_.demodule(moduled_expr{skeleton, 0, module});
    return make_normalizer_.make_normalizer(bind_map).normalize(framed_expr{global_skeleton, 0});
}

#endif
