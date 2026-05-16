#ifndef I_DECISION_GENERATOR_HPP
#define I_DECISION_GENERATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_decision_generator {
    virtual ~i_decision_generator() = default;
    virtual const resolution_lineage* generate() = 0;
};

#endif
