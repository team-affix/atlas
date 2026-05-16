#ifndef I_MHU_ELIMINATION_GENERATOR_HPP
#define I_MHU_ELIMINATION_GENERATOR_HPP

#include "i_elimination_generator.hpp"
#include "../value_objects/lineage.hpp"

struct i_mhu_elimination_generator : i_elimination_generator {
    virtual ~i_mhu_elimination_generator() = default;
    virtual bool add_head(const resolution_lineage*) = 0;
    virtual void remove_head(const resolution_lineage*) = 0;
};

#endif
