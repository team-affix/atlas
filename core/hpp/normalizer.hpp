#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include "expr.hpp"
#include "bind_map.hpp"

struct normalizer {
    normalizer(expr_pool&, bind_map&);
    const expr* operator()(const expr*);
#ifndef DEBUG
private:
#endif
    expr_pool& expr_pool_ref;
    bind_map& bind_map_ref;
};

#endif
