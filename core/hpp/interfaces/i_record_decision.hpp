#ifndef I_RECORD_DECISION_HPP
#define I_RECORD_DECISION_HPP

#include "../value_objects/lineage.hpp"

struct i_record_decision {
    virtual ~i_record_decision() = default;
    virtual void record_decision(const resolution_lineage*) = 0;
};

#endif

