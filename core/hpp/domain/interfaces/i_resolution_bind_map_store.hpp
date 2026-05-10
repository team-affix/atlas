#ifndef I_RESOLUTION_BIND_MAP_STORE_HPP
#define I_RESOLUTION_BIND_MAP_STORE_HPP

#include <memory>
#include "i_resolution_store.hpp"
#include "i_bind_map.hpp"

struct i_resolution_bind_map_store : i_resolution_store<std::unique_ptr<i_bind_map>> {
    virtual ~i_resolution_bind_map_store() = default;
};

#endif
