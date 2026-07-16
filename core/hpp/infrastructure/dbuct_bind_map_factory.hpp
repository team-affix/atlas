#ifndef DBUCT_BIND_MAP_FACTORY_HPP
#define DBUCT_BIND_MAP_FACTORY_HPP

#include "infrastructure/dbuct_bind_map.hpp"

template<typename IGlobalize>
struct dbuct_bind_map_factory {
    dbuct_bind_map_factory(IGlobalize& g);

    dbuct_bind_map<IGlobalize> make() const;

private:
    IGlobalize& globalizer_;
};

template<typename IGlobalize>
dbuct_bind_map_factory<IGlobalize>::dbuct_bind_map_factory(IGlobalize& g) : globalizer_(g) {}

template<typename IGlobalize>
dbuct_bind_map<IGlobalize> dbuct_bind_map_factory<IGlobalize>::make() const {
    return dbuct_bind_map<IGlobalize>{globalizer_};
}

#endif
