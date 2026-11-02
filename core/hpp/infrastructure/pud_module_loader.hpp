#ifndef PUD_MODULE_LOADER_HPP
#define PUD_MODULE_LOADER_HPP

#include <cstddef>
#include "value_objects/pud_frontier_change.hpp"

template<
    typename IModule,
    typename ILinkFunctorNames,
    typename IRegisterModule,
    typename ISeedFocus,
    typename IActivateModule,
    typename IApplyRegister,
    typename IAddPendingConflicts>
struct pud_module_loader {
    pud_module_loader(
        ILinkFunctorNames& link_functor_names,
        IRegisterModule& register_module,
        ISeedFocus& seed_focus,
        IActivateModule& activate_module,
        IApplyRegister& apply_register,
        IAddPendingConflicts& add_pending_conflicts);
    void load(IModule& module);
private:
    ILinkFunctorNames& link_functor_names_;
    IRegisterModule& register_module_;
    ISeedFocus& seed_focus_;
    IActivateModule& activate_module_;
    IApplyRegister& apply_register_;
    IAddPendingConflicts& add_pending_conflicts_;
};

#define PUD_MODULE_LOADER_TEMPLATE                                                     \
    template<typename IModule, typename ILinkFunctorNames, typename IRegisterModule,              \
             typename ISeedFocus, typename IActivateModule, typename IApplyRegister,              \
             typename IAddPendingConflicts>
#define PUD_MODULE_LOADER                                                              \
    pud_module_loader<IModule, ILinkFunctorNames, IRegisterModule, ISeedFocus,        \
        IActivateModule, IApplyRegister, IAddPendingConflicts>

PUD_MODULE_LOADER_TEMPLATE
PUD_MODULE_LOADER::pud_module_loader(
    ILinkFunctorNames& link_functor_names,
    IRegisterModule& register_module,
    ISeedFocus& seed_focus,
    IActivateModule& activate_module,
    IApplyRegister& apply_register,
    IAddPendingConflicts& add_pending_conflicts)
    : link_functor_names_(link_functor_names)
    , register_module_(register_module)
    , seed_focus_(seed_focus)
    , activate_module_(activate_module)
    , apply_register_(apply_register)
    , add_pending_conflicts_(add_pending_conflicts) {}

PUD_MODULE_LOADER_TEMPLATE
void PUD_MODULE_LOADER::load(IModule& module) {
    link_functor_names_.link(module);
    register_module_.register_module(module);
    seed_focus_.seed(module.get_module_id());
    const pud_frontier_change change = activate_module_.activate_module(module.get_module_id());
    apply_register_.apply_register(change);
    add_pending_conflicts_.add(change.refuted.size());
}

#undef PUD_MODULE_LOADER
#undef PUD_MODULE_LOADER_TEMPLATE

#endif
