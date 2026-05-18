#ifndef I_MHU_ELIMINATION_GENERATOR_HPP
#define I_MHU_ELIMINATION_GENERATOR_HPP

#include "i_elimination_generator.hpp"
#include "../value_objects/lineage.hpp"
#include "../value_objects/unify_head.hpp"

struct i_mhu_elimination_generator : i_elimination_generator {
    virtual ~i_mhu_elimination_generator() = default;
    virtual void add_head(const resolution_lineage*, unify_head, const std::unordered_set<uint32_t>&) = 0;
    virtual void remove_head(const resolution_lineage*) = 0;
};

#endif
