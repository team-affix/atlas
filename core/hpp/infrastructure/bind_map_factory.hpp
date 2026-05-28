#ifndef BIND_MAP_FACTORY_HPP
#define BIND_MAP_FACTORY_HPP

#include "interfaces/i_bind_map_factory.hpp"

struct bind_map_factory : i_bind_map_factory {
    std::unique_ptr<i_bind_map> make() const override;
};

#endif
