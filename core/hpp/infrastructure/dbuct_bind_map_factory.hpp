#ifndef DBUCT_BIND_MAP_FACTORY_HPP
#define DBUCT_BIND_MAP_FACTORY_HPP

#include "infrastructure/dbuct_bind_map.hpp"

// Delayed-backtracking variant of bind_map_factory: mints the local bind maps
// MHU uses for per-head unification. They share the solver's trail (abstracted as
// ILogTrailAction) and always journal, like every other bind map; MHU scopes a
// head's tentative unification with a trail savepoint instead of a journaling
// toggle.
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
    return dbuct_bind_map<ILogTrailAction>{globalizer_, trail_};
}

#endif
