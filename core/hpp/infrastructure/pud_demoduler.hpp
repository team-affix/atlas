#ifndef PUD_DEMODULER_HPP
#define PUD_DEMODULER_HPP

#include <functional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"

template<typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeVar, typename IMakeFunctor>
struct pud_demoduler {
    pud_demoduler(
        IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
        IMakeVar& make_var,
        IMakeFunctor& make_functor);
    const expr* demodule(moduled_expr term);
    const expr* demodule_functor(moduled_functor functor);
private:
    struct memo_key {
        const expr* skeleton;
        pud_module_id module;
        bool operator==(const memo_key&) const = default;
    };
    struct memo_hash {
        size_t operator()(const memo_key& key) const noexcept;
    };
    // One functor being rebuilt: its stored skeleton (none for the functor demodule_functor was handed), and the
    // arguments rebuilt so far.
    struct frame {
        const expr* skeleton;
        const expr::functor* functor;
        pud_module_id module;
        std::vector<const expr*> args;
    };

    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    std::unordered_map<memo_key, const expr*, memo_hash> memo_;
};

#define PUD_DEMODULER_TEMPLATE \
    template<typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeVar, typename IMakeFunctor>
#define PUD_DEMODULER pud_demoduler<IGetGlobalFunctorIdFromLocalFunctorId, IMakeVar, IMakeFunctor>

PUD_DEMODULER_TEMPLATE
PUD_DEMODULER::pud_demoduler(
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
    IMakeVar& make_var,
    IMakeFunctor& make_functor)
    : get_global_functor_id_(get_global_functor_id)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , memo_() {}

PUD_DEMODULER_TEMPLATE
size_t PUD_DEMODULER::memo_hash::operator()(const memo_key& key) const noexcept {
    const size_t skeleton_hash = std::hash<const expr*>{}(key.skeleton);
    return skeleton_hash ^ (std::hash<pud_module_id>{}(key.module) + 0x9e3779b97f4a7c15ull
        + (skeleton_hash << 6) + (skeleton_hash >> 2));
}

PUD_DEMODULER_TEMPLATE
const expr* PUD_DEMODULER::demodule(moduled_expr term) {
    const memo_key key{term.skeleton, term.module};
    const auto found = memo_.find(key);
    if (found != memo_.end())
        return found->second;
    const expr* demoduled = nullptr;
    if (const expr::var* variable = std::get_if<expr::var>(&term.skeleton->content))
        demoduled = make_var_.make_var(variable->index);
    else {
        const expr::functor& functor = std::get<expr::functor>(term.skeleton->content);
        demoduled = demodule_functor(moduled_functor{&functor, term.frame_offset, term.module});
    }
    memo_.emplace(key, demoduled);
    return demoduled;
}

// Walks with an explicit stack instead of recursing, so a term nested arbitrarily deep cannot exhaust the call
// stack. Every argument rebuilt on the way is memoized, as demodule memoizes a whole term.
PUD_DEMODULER_TEMPLATE
const expr* PUD_DEMODULER::demodule_functor(moduled_functor functor) {
    std::vector<frame> pending;
    pending.push_back(frame{.skeleton = nullptr, .functor = functor.functor, .module = functor.module, .args = {}});
    while (true) {
        frame& top = pending.back();
        const bool unfinished = top.args.size() < top.functor->args.size();
        if (unfinished) {
            const expr* arg = top.functor->args.at(top.args.size());
            const memo_key key{arg, top.module};
            const auto found = memo_.find(key);
            if (found != memo_.end()) {
                top.args.push_back(found->second);
                continue;
            }
            if (const expr::var* variable = std::get_if<expr::var>(&arg->content)) {
                const expr* made = make_var_.make_var(variable->index);
                memo_.emplace(key, made);
                top.args.push_back(made);
                continue;
            }
            const pud_module_id module = top.module;
            pending.push_back(frame{
                .skeleton = arg, .functor = &std::get<expr::functor>(arg->content), .module = module, .args = {}});
            continue;
        }
        const uint32_t global_functor_id =
            get_global_functor_id_.get_global_functor_id_from_local_functor_id(top.module, top.functor->id);
        const expr* rebuilt = make_functor_.make_functor(global_functor_id, top.args);
        const memo_key key{top.skeleton, top.module};
        pending.pop_back();
        if (pending.empty())
            return rebuilt;
        memo_.emplace(key, rebuilt);
        pending.back().args.push_back(rebuilt);
    }
}

#undef PUD_DEMODULER
#undef PUD_DEMODULER_TEMPLATE

#endif
