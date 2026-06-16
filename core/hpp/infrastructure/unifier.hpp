#ifndef UNIFIER_HPP
#define UNIFIER_HPP

#include "interfaces/i_unifier.hpp"
#include "interfaces/i_bind_map.hpp"

struct unifier : i_unifier {
    virtual ~unifier() = default;
    unifier(i_bind_map&);
    coroutine<uint32_t, bool> unify(framed_expr lhs, framed_expr rhs) override;
private:
    bool occurs_check(uint32_t global_key, framed_expr);
    i_bind_map& bind_map;
};

#endif
