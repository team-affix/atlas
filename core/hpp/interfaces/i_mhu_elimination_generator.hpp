#ifndef I_MHU_ELIMINATION_GENERATOR_HPP
#define I_MHU_ELIMINATION_GENERATOR_HPP

#include "i_elimination_generator.hpp"
#include "../value_objects/lineage.hpp"

struct i_mhu_elimination_generator : i_elimination_generator {
    virtual ~i_mhu_elimination_generator() = default;
    virtual bool try_add_head(const resolution_lineage*, const expr*, const expr*) = 0;
};

#endif
