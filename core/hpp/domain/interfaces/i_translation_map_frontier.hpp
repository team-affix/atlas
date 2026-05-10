#ifndef I_TRANSLATION_MAP_FRONTIER_HPP
#define I_TRANSLATION_MAP_FRONTIER_HPP

#include <memory>
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"
#include "i_translation_map.hpp"

struct i_translation_map_frontier : i_map<const resolution_lineage*, std::unique_ptr<i_translation_map>> {
    virtual ~i_translation_map_frontier() = default;
};

#endif
