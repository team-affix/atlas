#ifndef PUD_DEMODULER_HPP
#define PUD_DEMODULER_HPP

#include <functional>
#include <unordered_map>
#include <variant>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"

template<typename IDemoduleFunctor, typename IMakeVar>
struct pud_demoduler {
    pud_demoduler(IDemoduleFunctor& demodule_functor, IMakeVar& make_var);
    const expr* demodule(moduled_expr term);
private:
    struct memo_key {
        const expr* skeleton;
        pud_module_id module;
        bool operator==(const memo_key&) const = default;
    };
    struct memo_hash {
        size_t operator()(const memo_key& key) const noexcept;
    };

    IDemoduleFunctor& demodule_functor_;
    IMakeVar& make_var_;
    std::unordered_map<memo_key, const expr*, memo_hash> memo_;
};

template<typename IDemoduleFunctor, typename IMakeVar>
pud_demoduler<IDemoduleFunctor, IMakeVar>::pud_demoduler(IDemoduleFunctor& demodule_functor, IMakeVar& make_var)
    : demodule_functor_(demodule_functor)
    , make_var_(make_var)
    , memo_() {}

template<typename IDemoduleFunctor, typename IMakeVar>
size_t pud_demoduler<IDemoduleFunctor, IMakeVar>::memo_hash::operator()(const memo_key& key) const noexcept {
    const size_t skeleton_hash = std::hash<const expr*>{}(key.skeleton);
    return skeleton_hash ^ (std::hash<pud_module_id>{}(key.module) + 0x9e3779b97f4a7c15ull
        + (skeleton_hash << 6) + (skeleton_hash >> 2));
}

template<typename IDemoduleFunctor, typename IMakeVar>
const expr* pud_demoduler<IDemoduleFunctor, IMakeVar>::demodule(moduled_expr term) {
    const memo_key key{term.skeleton, term.module};
    const auto found = memo_.find(key);
    if (found != memo_.end())
        return found->second;
    const expr* demodulled = nullptr;
    if (const expr::var* variable = std::get_if<expr::var>(&term.skeleton->content))
        demodulled = make_var_.make_var(variable->index);
    else {
        const expr::functor& functor = std::get<expr::functor>(term.skeleton->content);
        demodulled = demodule_functor_.demodule(moduled_functor{&functor, term.frame_offset, term.module});
    }
    memo_.emplace(key, demodulled);
    return demodulled;
}

#endif
