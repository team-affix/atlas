#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <map>
#include "../value_objects/expr.hpp"
#include "trail.hpp"
#include <queue>

struct bind_map {
    bind_map(trail&);
    const expr* whnf(const expr*);
    bool unify(const expr*, const expr*, std::queue<uint32_t>&);
#ifndef DEBUG
private:
#endif
    bool occurs_check(uint32_t, const expr*);
    void bind(uint32_t, const expr*);
    
    trail& trail_ref;
    
    std::map<uint32_t, const expr*> bindings;
};

#endif
