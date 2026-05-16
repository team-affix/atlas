#ifndef I_ELIMINATION_PROCESSOR_HPP
#define I_ELIMINATION_PROCESSOR_HPP

#include "../value_objects/lineage.hpp"

struct i_elimination_processor {
    virtual ~i_elimination_processor() = default;
    virtual bool process(const resolution_lineage*) = 0;
};

#endif
