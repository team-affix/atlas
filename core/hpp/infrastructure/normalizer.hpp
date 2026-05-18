#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include "../interfaces/i_normalizer.hpp"
#include "../interfaces/i_expr_pool.hpp"
#include "../interfaces/i_bind_map.hpp"

struct normalizer : i_normalizer {
    normalizer(
        i_expr_pool&,
        i_bind_map&);
    const expr* normalize(const expr*) override;
private:
    i_expr_pool& expr_pool_ref;
    i_bind_map& bind_map_ref;
};

#endif
