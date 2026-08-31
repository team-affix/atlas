#ifndef VAR_COMPACTOR_HPP
#define VAR_COMPACTOR_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/expr.hpp"

template<typename IMakeFunctor, typename IMakeVar>
struct var_compactor {

    var_compactor(IMakeFunctor&, IMakeVar&);

    // Renumbers variables in a normalized expr to contiguous indices 0..K-1.
    const expr* compact_vars(
        const expr* e,
        std::unordered_map<uint32_t, uint32_t>& var_map,
        uint32_t& next_idx);

private:
    IMakeFunctor& make_functor_;
    IMakeVar&     make_var_;

};

template<typename IMF, typename IMV>
var_compactor<IMF, IMV>::var_compactor(IMF& mf, IMV& mv)
    : make_functor_(mf), make_var_(mv) {}

template<typename IMF, typename IMV>
const expr* var_compactor<IMF, IMV>::compact_vars(
    const expr* e,
    std::unordered_map<uint32_t, uint32_t>& var_map,
    uint32_t& next_idx) {

    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {

        auto [it, inserted] = var_map.emplace(v->index, next_idx);

        if (inserted) ++next_idx;

        return make_var_.make_var(it->second);
    }

    const expr::functor& f = std::get<expr::functor>(e->content);

    std::vector<const expr*> args;
    args.reserve(f.args.size());

    for (const expr* arg : f.args)
        args.push_back(compact_vars(arg, var_map, next_idx));

    return make_functor_.make_functor(f.id, args);
}

#endif
