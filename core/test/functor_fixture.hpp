#ifndef FUNCTOR_FIXTURE_HPP
#define FUNCTOR_FIXTURE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include "infrastructure/functor_names.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/rule.hpp"

struct test_functors {
    functor_names names;
    std::unordered_map<std::string, uint32_t> cache;
    uint32_t next_id = k_first_user_functor_id;

    uint32_t id(const char* s) {
        std::string key{s};
        if (key == "nil")
            return k_nil_functor_id;
        if (key == "cons")
            return k_cons_functor_id;
        if (auto it = cache.find(key); it != cache.end())
            return it->second;
        const uint32_t assigned = next_id++;
        names.set_name(assigned, key);
        cache.emplace(std::move(key), assigned);
        return assigned;
    }

    uint32_t id(const std::string& s) {
        return id(s.c_str());
    }
};

inline void raise_functor_id(const expr* skeleton, uint32_t& highest) {
    const expr::functor* functor = std::get_if<expr::functor>(&skeleton->content);
    if (functor == nullptr)
        return;
    if (functor->id + 1 > highest)
        highest = functor->id + 1;
    for (const expr* arg : functor->args)
        raise_functor_id(arg, highest);
}

inline void raise_rule_functor_ids(const rule& axiom, uint32_t& highest) {
    raise_functor_id(axiom.head, highest);
    for (const expr* goal : axiom.body)
        raise_functor_id(goal, highest);
}

// The shared test pool mints nil as 0 and cons as 1, then user functors from 2.
// The symbol table mints "[]" and "cons" as those same ids, so a module's name
// vector has to use that spelling and list every id up to the highest one used.
inline void install_shared_functor_names(pud_module& module, const test_functors& functors) {
    if (!module.functor_names().empty())
        return;
    uint32_t highest = functors.next_id;
    for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom)
        raise_rule_functor_ids(module.get_axiom(axiom), highest);
    for (pud_node_id node = module.count_axioms(); node < module.count_nodes(); ++node) {
        const pud_inference& inference = module.get_inference(node);
        for (const pud_substitution& substitution : inference.substitutions)
            raise_functor_id(substitution.value, highest);
        for (const expr* goal : inference.body_goals)
            raise_functor_id(goal, highest);
    }
    for (uint32_t id = 0; id < highest; ++id) {
        if (id == k_nil_functor_id)
            module.append_functor_name("[]");
        else if (id == k_cons_functor_id)
            module.append_functor_name("cons");
        else if (functors.names.is_named(id))
            module.append_functor_name(functors.names.name(id));
        else
            module.append_functor_name("functor-" + std::to_string(id));
    }
}

#endif
