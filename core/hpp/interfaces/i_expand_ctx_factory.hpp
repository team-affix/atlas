#ifndef I_EXPAND_CTX_FACTORY_HPP
#define I_EXPAND_CTX_FACTORY_HPP

#include "../interfaces/i_factory.hpp"
#include "../value_objects/expand_ctx.hpp"

struct i_expand_ctx_factory :
    i_factory<
        expand_ctx,
        const std::vector<const expr*>&,
        std::unordered_map<uint32_t, uint32_t>&> {
    virtual ~i_expand_ctx_factory() = default;
};

#endif
