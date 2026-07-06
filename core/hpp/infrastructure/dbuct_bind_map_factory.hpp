#ifndef DBUCT_BIND_MAP_FACTORY_HPP
#define DBUCT_BIND_MAP_FACTORY_HPP

#include "infrastructure/dbuct_bind_map.hpp"

// Delayed-backtracking variant of bind_map_factory: mints the local bind maps MHU
// uses for per-head unification, sharing the solver's trail and always journalling.
template<typename IGlobalize, typename ILogTrailAction>
struct dbuct_bind_map_factory {
    dbuct_bind_map_factory(IGlobalize& g, ILogTrailAction& t);
    dbuct_bind_map<IGlobalize, ILogTrailAction> make() const;
private:
    IGlobalize& globalizer_;
    ILogTrailAction& trail_;
};

template<typename IGlobalize, typename ILogTrailAction>
dbuct_bind_map_factory<IGlobalize, ILogTrailAction>::dbuct_bind_map_factory(IGlobalize& g, ILogTrailAction& t)
    : globalizer_(g), trail_(t) {}

template<typename IGlobalize, typename ILogTrailAction>
dbuct_bind_map<IGlobalize, ILogTrailAction> dbuct_bind_map_factory<IGlobalize, ILogTrailAction>::make() const {
    return dbuct_bind_map<IGlobalize, ILogTrailAction>{globalizer_, trail_};
}

#endif
