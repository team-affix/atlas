#ifndef EXPR_POOL_UBS_FACTORIES_HPP
#define EXPR_POOL_UBS_FACTORIES_HPP

#include <cstdint>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "value_objects/expr.hpp"

struct expr_pool_ubs_var_factory {
    expr_pool_ubs_var_factory(expr_pool& pool);
    const expr* make(uint32_t global_index);

private:
    expr_pool& pool_;
};

struct expr_pool_ubs_functor_factory {
    expr_pool_ubs_functor_factory(expr_pool& pool);
    const expr* make(uint32_t id, const std::vector<const expr*>& args);

private:
    expr_pool& pool_;
};

#endif
