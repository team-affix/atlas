#ifndef BIND_MAP_FACTORY_HPP
#define BIND_MAP_FACTORY_HPP

#include "infrastructure/bind_map.hpp"

struct bind_map_factory {
    explicit bind_map_factory(globalizer&);
    bind_map make() const;
private:
    globalizer& globalizer_;
};

#endif
