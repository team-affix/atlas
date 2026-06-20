#ifndef BIND_MAP_FACTORY_HPP
#define BIND_MAP_FACTORY_HPP

#include "infrastructure/bind_map.hpp"

struct bind_map_factory {
    explicit bind_map_factory(globalizer&);
    bind_map make() const;
private:
    globalizer& globalizer_;
};

inline bind_map_factory::bind_map_factory(globalizer& g) : globalizer_(g) {}

inline bind_map bind_map_factory::make() const {
    return bind_map{globalizer_};
}

#endif
