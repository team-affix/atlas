#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <map>
#include "expr.hpp"
#include "topic.hpp"

struct bind_map {
    bind_map();
    const expr* whnf(const expr*);
    bool unify(const expr*, const expr*);
#ifndef DEBUG
private:
#endif
    bool occurs_check(uint32_t, const expr*);
    void bind(uint32_t, const expr*);
    
    trail& trail_ref;
    topic<uint32_t>& rep_changed_topic;
    
    std::map<uint32_t, const expr*> bindings;
};

#endif
