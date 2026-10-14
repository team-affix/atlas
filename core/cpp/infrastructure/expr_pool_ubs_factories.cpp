#include "infrastructure/expr_pool_ubs_factories.hpp"

expr_pool_ubs_var_factory::expr_pool_ubs_var_factory(expr_pool& pool)
    : pool_(pool) {}

const expr* expr_pool_ubs_var_factory::make(uint32_t global_index) {
    return pool_.make_var(global_index);
}

expr_pool_ubs_functor_factory::expr_pool_ubs_functor_factory(expr_pool& pool)
    : pool_(pool) {}

const expr* expr_pool_ubs_functor_factory::make(uint32_t id, const std::vector<const expr*>& args) {
    return pool_.make_functor(id, args);
}
