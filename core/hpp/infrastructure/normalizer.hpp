#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include "../domain/interfaces/i_normalizer.hpp"
#include "../domain/interfaces/i_expr_pool.hpp"
#include "../domain/interfaces/i_bind_map.hpp"

struct normalizer : i_normalizer {
    normalizer();
    const expr* normalize(const expr*) override;
#ifndef DEBUG
private:
#endif
    i_expr_pool& expr_pool_ref;
    i_bind_map& bind_map_ref;
};

#endif
