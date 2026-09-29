#ifndef EXPR_LIFTER_HPP
#define EXPR_LIFTER_HPP

#include "value_objects/expr.hpp"

template<typename IMakeVar, typename IMakeFunctor>
struct expr_lifter {
    expr_lifter(IMakeVar& make_var, IMakeFunctor& make_functor);
    const expr* lift(const expr* e, uint32_t shift_by);
private:
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
};

template<typename IMV, typename IMF>
expr_lifter<IMV, IMF>::expr_lifter(IMV& make_var, IMF& make_functor)
    : make_var_(make_var)
{
}

template<typename IMV, typename IMF>
const expr* expr_lifter<IMV, IMF>::lift(const expr* e, uint32_t shift_by) {
    if (const expr::var* var = std::get_if<expr::var>(&e->content)) {
        return make_var_.make(var->index + shift_by);
    }

    const expr::functor& functor = std::get<expr::functor>(e->content);
    
    std::vector<const expr*> lifted_args;

    for (const expr* arg : functor.args) {
        lifted_args.push_back(lift(arg, shift_by));
    }

    return make_functor_.make(functor.id, lifted_args);
}

#endif
