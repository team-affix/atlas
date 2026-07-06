#ifndef BIND_MAP_FACTORY_HPP
#define BIND_MAP_FACTORY_HPP

#include "infrastructure/bind_map.hpp"

template<typename IGlobalize>
struct bind_map_factory {
    explicit bind_map_factory(IGlobalize&);
    bind_map<IGlobalize> make() const;
private:
    IGlobalize& globalizer_;
};

template<typename IGlobalize>
bind_map_factory<IGlobalize>::bind_map_factory(IGlobalize& g) : globalizer_(g) {}

template<typename IGlobalize>
bind_map<IGlobalize> bind_map_factory<IGlobalize>::make() const {
    return bind_map<IGlobalize>{globalizer_};
}

#endif
