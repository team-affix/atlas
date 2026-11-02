#ifndef PUD_UNIFIER_FACTORY_HPP
#define PUD_UNIFIER_FACTORY_HPP

#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IGetGlobalFunctorIdFromLocalFunctorId, typename ICycleDetector>
struct pud_unifier_factory {
    using unifier_t = pud_unifier<
        IGlobalize, ubs_bind_map, ubs_bind_map, ubs_bind_map, ICycleDetector, IMakeFunctor,
        IGetGlobalFunctorIdFromLocalFunctorId>;

    pud_unifier_factory(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id);
    unifier_t make_unifier(ubs_bind_map& bind_map, ICycleDetector& cycle_detector) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id_;
};

#define PUD_UNIFIER_FACTORY_TEMPLATE                                                             \
    template<typename IGlobalize, typename IMakeFunctor, typename IGetGlobalFunctorIdFromLocalFunctorId, \
             typename ICycleDetector>
#define PUD_UNIFIER_FACTORY \
    pud_unifier_factory<IGlobalize, IMakeFunctor, IGetGlobalFunctorIdFromLocalFunctorId, ICycleDetector>

PUD_UNIFIER_FACTORY_TEMPLATE
PUD_UNIFIER_FACTORY::pud_unifier_factory(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , get_global_functor_id_(get_global_functor_id) {}

PUD_UNIFIER_FACTORY_TEMPLATE
typename PUD_UNIFIER_FACTORY::unifier_t PUD_UNIFIER_FACTORY::make_unifier(
    ubs_bind_map& bind_map, ICycleDetector& cycle_detector) const {
    return unifier_t{globalize_, bind_map, bind_map, bind_map, cycle_detector, make_functor_, get_global_functor_id_};
}

#undef PUD_UNIFIER_FACTORY
#undef PUD_UNIFIER_FACTORY_TEMPLATE

#endif
