#ifndef DBUCT_BIND_MAP_FACTORY_HPP
#define DBUCT_BIND_MAP_FACTORY_HPP

#include "infrastructure/dbuct_bind_map.hpp"

// Delayed-backtracking variant of bind_map_factory: mints the local bind maps
// MHU uses for per-head unification. They share the solver's trail (abstracted as
// ILogTrailAction) but start with journaling OFF (creation-time binds are
// subsumed by the head arena's undo); MHU calls enable_journaling() once a head
// commits.
template<typename ILogTrailAction>
struct dbuct_bind_map_factory {
    dbuct_bind_map_factory(globalizer& g, ILogTrailAction& t);
    dbuct_bind_map<ILogTrailAction> make() const;
private:
    globalizer& globalizer_;
    ILogTrailAction& trail_;
};

template<typename ILogTrailAction>
dbuct_bind_map_factory<ILogTrailAction>::dbuct_bind_map_factory(globalizer& g, ILogTrailAction& t)
    : globalizer_(g), trail_(t) {}

template<typename ILogTrailAction>
dbuct_bind_map<ILogTrailAction> dbuct_bind_map_factory<ILogTrailAction>::make() const {
    return dbuct_bind_map<ILogTrailAction>{globalizer_, trail_, false};
}

#endif
