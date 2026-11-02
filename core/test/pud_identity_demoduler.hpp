#ifndef PUD_IDENTITY_DEMODULER_HPP
#define PUD_IDENTITY_DEMODULER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"

struct pud_identity_global_functor_ids {
    uint32_t get_global_functor_id_from_local_functor_id(pud_module_id module, uint32_t local_functor_id) const;
};

inline uint32_t pud_identity_global_functor_ids::get_global_functor_id_from_local_functor_id(
    pud_module_id, uint32_t local_functor_id) const {
    return local_functor_id;
}

struct pud_identity_functor_relinker {
    uint32_t relink(pud_module_id from_module, uint32_t local_functor_id, pud_module_id to_module);
};

inline uint32_t pud_identity_functor_relinker::relink(pud_module_id, uint32_t local_functor_id, pud_module_id) {
    return local_functor_id;
}

template<typename IMakeFunctor>
struct pud_copy_functor_demoduler {
    pud_copy_functor_demoduler(IMakeFunctor& make_functor);
    const expr* demodule_functor(moduled_functor functor);
private:
    IMakeFunctor& make_functor_;
};

template<typename IMakeFunctor>
pud_copy_functor_demoduler<IMakeFunctor>::pud_copy_functor_demoduler(IMakeFunctor& make_functor)
    : make_functor_(make_functor) {}

template<typename IMakeFunctor>
const expr* pud_copy_functor_demoduler<IMakeFunctor>::demodule_functor(moduled_functor functor) {
    return make_functor_.make_functor(functor.functor->id, functor.functor->args);
}

#endif
