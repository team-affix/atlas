#ifndef I_MULTIHEAD_UNIFIER_HPP
#define I_MULTIHEAD_UNIFIER_HPP

#include "../value_objects/lineage.hpp"

struct i_multihead_unifier {
    virtual ~i_multihead_unifier() = default;
    virtual void goal_activated(const goal_lineage*) = 0;
    virtual void goal_deactivated(const goal_lineage*) = 0;
    virtual void accept(const resolution_lineage*) = 0;
};

#endif
