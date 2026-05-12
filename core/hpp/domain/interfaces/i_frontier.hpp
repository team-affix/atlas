#ifndef I_FRONTIER_HPP
#define I_FRONTIER_HPP

#include <memory>
#include "i_map.hpp"
#include "i_goal.hpp"
#include "../value_objects/lineage.hpp"

struct i_frontier : i_map<const goal_lineage*, std::unique_ptr<i_goal>> {
    virtual ~i_frontier() = default;
};

#endif
