#ifndef I_UNIFIER_FRONTIER_HPP
#define I_UNIFIER_FRONTIER_HPP

#include <memory>
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"
#include "i_unifier.hpp"

struct i_unifier_frontier : i_map<const resolution_lineage*, std::unique_ptr<i_unifier>> {
    virtual ~i_unifier_frontier() = default;
};

#endif
