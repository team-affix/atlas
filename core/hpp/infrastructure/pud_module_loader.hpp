#ifndef PUD_MODULE_LOADER_HPP
#define PUD_MODULE_LOADER_HPP

#include <cstddef>
#include "value_objects/pud_frontier_change.hpp"

template<
    typename IModule,
    typename ILinkFunctorNames,
    typename IRegisterModule,
    typename IActivateModule,
    typename IApplyRegister,
    typename IAddPendingConflicts,
    typename IDropStoredSolutions>
struct pud_module_loader {
    pud_module_loader(
        ILinkFunctorNames& link_functor_names,
        IRegisterModule& register_module,
        IActivateModule& activate_module,
        IApplyRegister& apply_register,
        IAddPendingConflicts& add_pending_conflicts,
        IDropStoredSolutions& drop_stored_solutions);
    void load(IModule& module);
private:
    ILinkFunctorNames& link_functor_names_;
    IRegisterModule& register_module_;
    IActivateModule& activate_module_;
    IApplyRegister& apply_register_;
    IAddPendingConflicts& add_pending_conflicts_;
    IDropStoredSolutions& drop_stored_solutions_;
};

#define PUD_MODULE_LOADER_TEMPLATE                                                     \
    template<typename IModule, typename ILinkFunctorNames, typename IRegisterModule,              \
             typename IActivateModule, typename IApplyRegister,              \
             typename IAddPendingConflicts, typename IDropStoredSolutions>
#define PUD_MODULE_LOADER                                                              \
    pud_module_loader<IModule, ILinkFunctorNames, IRegisterModule,        \
        IActivateModule, IApplyRegister, IAddPendingConflicts, IDropStoredSolutions>

PUD_MODULE_LOADER_TEMPLATE
PUD_MODULE_LOADER::pud_module_loader(
    ILinkFunctorNames& link_functor_names,
    IRegisterModule& register_module,
    IActivateModule& activate_module,
    IApplyRegister& apply_register,
    IAddPendingConflicts& add_pending_conflicts,
    IDropStoredSolutions& drop_stored_solutions)
    : link_functor_names_(link_functor_names)
    , register_module_(register_module)
    , activate_module_(activate_module)
    , apply_register_(apply_register)
    , add_pending_conflicts_(add_pending_conflicts)
    , drop_stored_solutions_(drop_stored_solutions) {}

PUD_MODULE_LOADER_TEMPLATE
void PUD_MODULE_LOADER::load(IModule& module) {
    link_functor_names_.link(module);
    register_module_.register_module(module);
    const pud_frontier_change change = activate_module_.activate_module(module.get_module_id());
    apply_register_.apply_register(change);
    add_pending_conflicts_.add(change.refuted.size());
    drop_stored_solutions_.drop();
}

#undef PUD_MODULE_LOADER
#undef PUD_MODULE_LOADER_TEMPLATE

#endif
