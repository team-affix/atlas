#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <map>
#include <unordered_set>
#include "expr.hpp"

struct bind_map {
    bind_map(trail&);
    const expr* whnf(const expr*);
    bool unify(const expr*, const expr*);
    std::unordered_set<uint32_t> flush_changed_reps();
#ifndef DEBUG
private:
#endif
    bool occurs_check(uint32_t, const expr*);
    void bind(uint32_t, const expr*);
    std::map<uint32_t, const expr*> bindings;
    trail& trail_ref;
    std::unordered_set<uint32_t> changed_reps;
};

#endif
