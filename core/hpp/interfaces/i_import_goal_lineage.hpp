#ifndef I_IMPORT_GOAL_LINEAGE_HPP
#define I_IMPORT_GOAL_LINEAGE_HPP

#include "../value_objects/lineage.hpp"

struct i_import_goal_lineage {
    virtual ~i_import_goal_lineage() = default;
    virtual const goal_lineage* import(const goal_lineage*) = 0;
};

#endif

