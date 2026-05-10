#ifndef I_RESOLUTION_TRANSLATION_MAP_STORE_HPP
#define I_RESOLUTION_TRANSLATION_MAP_STORE_HPP

#include <memory>
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"
#include "i_translation_map.hpp"

struct i_resolution_translation_map_store : i_map<const resolution_lineage*, std::unique_ptr<i_translation_map>> {
    virtual ~i_resolution_translation_map_store() = default;
};

#endif
