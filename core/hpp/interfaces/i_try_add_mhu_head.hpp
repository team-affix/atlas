#ifndef I_TRY_ADD_MHU_HEAD_HPP
#define I_TRY_ADD_MHU_HEAD_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/expr.hpp"

struct i_try_add_mhu_head {
    virtual ~i_try_add_mhu_head() = default;
    virtual bool try_add_head(const resolution_lineage*, const expr*, const expr*) = 0;
};

#endif
