#ifndef OVERLAY_BIND_MAP_FACTORY_HPP
#define OVERLAY_BIND_MAP_FACTORY_HPP

#include "interfaces/i_overlay_bind_map_factory.hpp"

struct overlay_bind_map_factory : i_overlay_bind_map_factory {
    std::unique_ptr<i_bind_map> make(i_bind_map& local, i_bind_map& remote) const override;
};

#endif
