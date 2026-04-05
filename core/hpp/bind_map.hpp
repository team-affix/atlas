#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <map>
#include "expr.hpp"

struct bind_map {
    bind_map(trail&, registry<expr>&);
    expr_id whnf(expr_id);
    bool unify(expr_id, expr_id);
#ifndef DEBUG
private:
#endif
    bool occurs_check(uint32_t, expr_id);
    void bind(uint32_t, expr_id);
    std::map<uint32_t, expr_id> bindings;
    trail& trail_ref;
    registry<expr>& er;
};

#endif
