#ifndef UNIFIER_HPP
#define UNIFIER_HPP

#include "interfaces/i_unifier.hpp"
#include "interfaces/i_bind_map.hpp"

struct unifier : i_unifier {
    virtual ~unifier() = default;
    unifier(i_bind_map&);
    coroutine<uint32_t, bool> unify(const expr*, const expr*) override;
private:
    bool occurs_check(uint32_t, const expr*);
    i_bind_map& bind_map;
};

#endif
