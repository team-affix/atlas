#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <map>
#include <functional>
#include "expr.hpp"

struct bind_map {
    bind_map(trail&);
    const expr* whnf(const expr*);
    bool unify(const expr*, const expr*);
    void set_representative_changed_callback(std::function<void(uint32_t)>);
#ifndef DEBUG
private:
#endif
    bool occurs_check(uint32_t, const expr*);
    void bind(uint32_t, const expr*);
    std::map<uint32_t, const expr*> bindings;
    trail& trail_ref;
    std::function<void(uint32_t)> representative_changed_callback;
};

#endif
