#ifndef PUD_LODESTAR_MODULE_REGISTRAR_HPP
#define PUD_LODESTAR_MODULE_REGISTRAR_HPP

#include <cstddef>
#include "value_objects/pud_frontier_change.hpp"

template<
    typename IModule,
    typename ILinkFunctorNames,
    typename IRegisterModule,
    typename ISeedFocus,
    typename IActivateModule,
    typename IApplyRegister,
    typename IGetPendingConflicts,
    typename ISetPendingConflicts>
struct pud_lodestar_module_registrar {
    pud_lodestar_module_registrar(
        ILinkFunctorNames& link_functor_names,
        IRegisterModule& register_module,
        ISeedFocus& seed_focus,
        IActivateModule& activate_module,
        IApplyRegister& apply_register,
        IGetPendingConflicts& get_pending_conflicts,
        ISetPendingConflicts& set_pending_conflicts);
    void register_module(IModule& module);
private:
    ILinkFunctorNames& link_functor_names_;
    IRegisterModule& register_module_;
    ISeedFocus& seed_focus_;
    IActivateModule& activate_module_;
    IApplyRegister& apply_register_;
    IGetPendingConflicts& get_pending_conflicts_;
    ISetPendingConflicts& set_pending_conflicts_;
};

#define PUD_LODESTAR_MODULE_REGISTRAR_TEMPLATE                                                     \
    template<typename IModule, typename ILinkFunctorNames, typename IRegisterModule,              \
             typename ISeedFocus, typename IActivateModule, typename IApplyRegister,              \
             typename IGetPendingConflicts, typename ISetPendingConflicts>
#define PUD_LODESTAR_MODULE_REGISTRAR                                                              \
    pud_lodestar_module_registrar<IModule, ILinkFunctorNames, IRegisterModule, ISeedFocus,        \
        IActivateModule, IApplyRegister, IGetPendingConflicts, ISetPendingConflicts>

PUD_LODESTAR_MODULE_REGISTRAR_TEMPLATE
PUD_LODESTAR_MODULE_REGISTRAR::pud_lodestar_module_registrar(
    ILinkFunctorNames& link_functor_names,
    IRegisterModule& register_module,
    ISeedFocus& seed_focus,
    IActivateModule& activate_module,
    IApplyRegister& apply_register,
    IGetPendingConflicts& get_pending_conflicts,
    ISetPendingConflicts& set_pending_conflicts)
    : link_functor_names_(link_functor_names)
    , register_module_(register_module)
    , seed_focus_(seed_focus)
    , activate_module_(activate_module)
    , apply_register_(apply_register)
    , get_pending_conflicts_(get_pending_conflicts)
    , set_pending_conflicts_(set_pending_conflicts) {}

PUD_LODESTAR_MODULE_REGISTRAR_TEMPLATE
void PUD_LODESTAR_MODULE_REGISTRAR::register_module(IModule& module) {
    link_functor_names_.link(module);
    register_module_.register_module(module);
    seed_focus_.seed(module.get_module_id());
    const pud_frontier_change change = activate_module_.activate_module(module.get_module_id());
    apply_register_.apply_register(change);
    set_pending_conflicts_.set(get_pending_conflicts_.get() + change.refuted.size());
}

#undef PUD_LODESTAR_MODULE_REGISTRAR
#undef PUD_LODESTAR_MODULE_REGISTRAR_TEMPLATE

#endif
