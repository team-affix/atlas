#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <map>
#include "expr.hpp"

struct bind_map {
    bind_map(trail&);
    const expr* whnf(const expr*);
    bool unify(const expr*, const expr*);
#ifndef DEBUG
private:
#endif
    bool occurs_check(uint32_t, const expr*);
    void bind(uint32_t, const expr*);
    std::map<uint32_t, const expr*> bindings;
    trail& trail_ref;
};

#endif
