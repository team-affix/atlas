#ifndef PUD_FUNCTOR_LINKER_HPP
#define PUD_FUNCTOR_LINKER_HPP

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>
#include "infrastructure/pud_module.hpp"
#include "value_objects/expr.hpp"

template<typename IInternFunctorName, typename IInsertLink>
struct pud_functor_linker {
    pud_functor_linker(IInternFunctorName& intern_functor_name, IInsertLink& insert_link);
    void link(pud_module& module);
private:
    void link_unnamed_functor(
        pud_module_id module,
        uint32_t named_count,
        const expr* skeleton,
        std::unordered_set<uint32_t>& linked);

    IInternFunctorName& intern_functor_name_;
    IInsertLink& insert_link_;
};

template<typename IInternFunctorName, typename IInsertLink>
pud_functor_linker<IInternFunctorName, IInsertLink>::pud_functor_linker(
    IInternFunctorName& intern_functor_name, IInsertLink& insert_link)
    : intern_functor_name_(intern_functor_name)
    , insert_link_(insert_link) {}

template<typename IInternFunctorName, typename IInsertLink>
void pud_functor_linker<IInternFunctorName, IInsertLink>::link(pud_module& module) {
    const std::vector<std::string>& names = module.functor_names();
    const uint32_t named_count = static_cast<uint32_t>(names.size());
    for (uint32_t local_functor_id = 0; local_functor_id < named_count; ++local_functor_id) {
        const uint32_t global_functor_id = intern_functor_name_.intern_functor_name(names[local_functor_id]);
        insert_link_.insert_link(module.get_module_id(), local_functor_id, global_functor_id);
    }
    std::unordered_set<uint32_t> linked;
    const pud_module_id module_id = module.get_module_id();
    for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom) {
        const rule& axiom_rule = module.get_axiom(axiom);
        link_unnamed_functor(module_id, named_count, axiom_rule.head, linked);
        for (const expr* goal : axiom_rule.body)
            link_unnamed_functor(module_id, named_count, goal, linked);
    }
    for (pud_node_id node = module.count_axioms(); node < module.count_nodes(); ++node) {
        const pud_inference& inference = module.get_inference(node);
        for (const pud_substitution& substitution : inference.substitutions)
            link_unnamed_functor(module_id, named_count, substitution.value, linked);
        for (const expr* goal : inference.body_goals)
            link_unnamed_functor(module_id, named_count, goal, linked);
    }
}

template<typename IInternFunctorName, typename IInsertLink>
void pud_functor_linker<IInternFunctorName, IInsertLink>::link_unnamed_functor(
    pud_module_id module,
    uint32_t named_count,
    const expr* skeleton,
    std::unordered_set<uint32_t>& linked) {
    const expr::functor* functor = std::get_if<expr::functor>(&skeleton->content);
    if (functor == nullptr)
        return;
    const bool unnamed = functor->id >= named_count;
    if (unnamed && linked.insert(functor->id).second)
        insert_link_.insert_link(module, functor->id, functor->id);
    for (const expr* arg : functor->args)
        link_unnamed_functor(module, named_count, arg, linked);
}

#endif
