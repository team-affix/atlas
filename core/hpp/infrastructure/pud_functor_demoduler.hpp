#ifndef PUD_FUNCTOR_DEMODULER_HPP
#define PUD_FUNCTOR_DEMODULER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"

template<typename IDemodule, typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeFunctor>
struct pud_functor_demoduler {
    pud_functor_demoduler(
        IDemodule& demodule,
        IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
        IMakeFunctor& make_functor);
    const expr* demodule(moduled_functor functor);
private:
    IDemodule& demodule_;
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id_;
    IMakeFunctor& make_functor_;
};

template<typename IDemodule, typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeFunctor>
pud_functor_demoduler<IDemodule, IGetGlobalFunctorIdFromLocalFunctorId, IMakeFunctor>::pud_functor_demoduler(
    IDemodule& demodule,
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
    IMakeFunctor& make_functor)
    : demodule_(demodule)
    , get_global_functor_id_(get_global_functor_id)
    , make_functor_(make_functor) {}

template<typename IDemodule, typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeFunctor>
const expr* pud_functor_demoduler<IDemodule, IGetGlobalFunctorIdFromLocalFunctorId, IMakeFunctor>::demodule(
    moduled_functor functor) {
    std::vector<const expr*> args;
    args.reserve(functor.functor->args.size());
    for (const expr* arg : functor.functor->args)
        args.push_back(demodule_.demodule(moduled_expr{arg, functor.frame_offset, functor.module}));
    const uint32_t global_functor_id = get_global_functor_id_.get_global_functor_id_from_local_functor_id(
        functor.module, functor.functor->id);
    return make_functor_.make_functor(global_functor_id, args);
}

#endif
