#ifndef I_MULTIHEAD_UNIFIER_HPP
#define I_MULTIHEAD_UNIFIER_HPP

#include "../value_objects/lineage.hpp"

struct i_multihead_unifier {
    virtual ~i_multihead_unifier() = default;
    virtual void add_head(const resolution_lineage*) = 0;
    virtual void remove_head(const resolution_lineage*) = 0;
    virtual void init_accept_head(const resolution_lineage*) = 0;
    virtual void resume_accept_head() = 0;
};

#endif
