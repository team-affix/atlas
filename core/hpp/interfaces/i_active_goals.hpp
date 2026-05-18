#ifndef I_ACTIVE_GOALS_HPP
#define I_ACTIVE_GOALS_HPP

#include "../interfaces/i_visitor.hpp"
#include "../value_objects/lineage.hpp"

struct i_active_goals {
    virtual ~i_active_goals() = default;
    virtual void insert(const goal_lineage*) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual bool contains(const goal_lineage*) const = 0;
    virtual void accept(i_visitor<const goal_lineage*>&) = 0;
    virtual bool empty() const = 0;
};

#endif
