#ifndef EXPR_LIFTER_HPP
#define EXPR_LIFTER_HPP

#include <cstdint>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"

template<typename IMakeVar, typename IMakeFunctor>
struct expr_lifter {
    expr_lifter(IMakeVar& make_var, IMakeFunctor& make_functor);
    const expr* lift(const expr* expression, uint32_t amount);
private:
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
};

template<typename IMV, typename IMF>
expr_lifter<IMV, IMF>::expr_lifter(IMV& make_var, IMF& make_functor)
    : make_var_(make_var)
    , make_functor_(make_functor) {}

template<typename IMV, typename IMF>
const expr* expr_lifter<IMV, IMF>::lift(const expr* expression, uint32_t amount) {
    if (const expr::var* var = std::get_if<expr::var>(&expression->content))
        return make_var_.make_var(var->index + amount);

    const expr::functor& functor = std::get<expr::functor>(expression->content);
    std::vector<const expr*> lifted_args;
    lifted_args.reserve(functor.args.size());
    for (const expr* arg : functor.args)
        lifted_args.push_back(lift(arg, amount));
    return make_functor_.make_functor(functor.id, lifted_args);
}

#endif
