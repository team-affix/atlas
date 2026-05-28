#ifndef I_GENERATE_DECISION_HPP
#define I_GENERATE_DECISION_HPP

#include "../value_objects/lineage.hpp"

struct i_generate_decision {
    virtual ~i_generate_decision() = default;
    virtual const resolution_lineage* generate() = 0;
};

#endif
