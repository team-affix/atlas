#ifndef I_RESOLUTION_BIND_MAP_STORE_HPP
#define I_RESOLUTION_BIND_MAP_STORE_HPP

#include <memory>
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"
#include "i_unifier.hpp"

struct i_resolution_bind_map_store : i_map<const resolution_lineage*, std::unique_ptr<i_unifier>> {
    virtual ~i_resolution_bind_map_store() = default;
};

#endif
