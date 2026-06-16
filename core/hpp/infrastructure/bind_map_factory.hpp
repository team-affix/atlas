#ifndef BIND_MAP_FACTORY_HPP
#define BIND_MAP_FACTORY_HPP

#include "interfaces/i_bind_map_factory.hpp"
#include "interfaces/i_globalizer.hpp"

struct bind_map_factory : i_bind_map_factory {
    explicit bind_map_factory(i_globalizer&);
    std::unique_ptr<i_bind_map> make() const override;
private:
    i_globalizer& globalizer_;
};

#endif
