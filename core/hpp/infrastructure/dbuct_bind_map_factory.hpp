#ifndef DBUCT_BIND_MAP_FACTORY_HPP
#define DBUCT_BIND_MAP_FACTORY_HPP

#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/trail.hpp"

// Delayed-backtracking variant of bind_map_factory: mints the local bind maps
// MHU uses for per-head unification. They share the solver's trail but start
// with journaling OFF (creation-time binds are subsumed by the head arena's
// undo); MHU calls enable_journaling() once a head commits.
struct dbuct_bind_map_factory {
    dbuct_bind_map_factory(globalizer& g, trail& t);
    dbuct_bind_map make() const;
private:
    globalizer& globalizer_;
    trail& trail_;
};

inline dbuct_bind_map_factory::dbuct_bind_map_factory(globalizer& g, trail& t) : globalizer_(g), trail_(t) {}
inline dbuct_bind_map dbuct_bind_map_factory::make() const { return dbuct_bind_map{globalizer_, trail_, false}; }

#endif
