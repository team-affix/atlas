#ifndef PUD_FUNCTOR_LINKER_HPP
#define PUD_FUNCTOR_LINKER_HPP

#include <cstdint>
#include <string>
#include <vector>

// Links a module's functor names to global functor ids. Any module that lists its names and its id can be linked:
// one held in memory or one paged from disk.
template<typename IInternFunctorName, typename IInsertLink>
struct pud_functor_linker {
    pud_functor_linker(IInternFunctorName& intern_functor_name, IInsertLink& insert_link);
    template<typename IModule>
    void link(const IModule& module);
private:
    IInternFunctorName& intern_functor_name_;
    IInsertLink& insert_link_;
};

template<typename IInternFunctorName, typename IInsertLink>
pud_functor_linker<IInternFunctorName, IInsertLink>::pud_functor_linker(
    IInternFunctorName& intern_functor_name, IInsertLink& insert_link)
    : intern_functor_name_(intern_functor_name)
    , insert_link_(insert_link) {}

template<typename IInternFunctorName, typename IInsertLink>
template<typename IModule>
void pud_functor_linker<IInternFunctorName, IInsertLink>::link(const IModule& module) {
    const std::vector<std::string>& names = module.functor_names();
    for (uint32_t local_functor_id = 0; local_functor_id < names.size(); ++local_functor_id) {
        const uint32_t global_functor_id = intern_functor_name_.intern_functor_name(names[local_functor_id]);
        insert_link_.insert_link(module.get_module_id(), local_functor_id, global_functor_id);
    }
}

#endif
