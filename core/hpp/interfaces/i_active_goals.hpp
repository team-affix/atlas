#ifndef I_ACTIVE_GOALS_HPP
#define I_ACTIVE_GOALS_HPP

#include "../value_objects/lineage.hpp"
#include "../utility/state_machine.hpp"

struct i_active_goals {
    virtual ~i_active_goals() = default;
    virtual void insert(const goal_lineage*) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual bool contains(const goal_lineage*) const = 0;
    virtual state_machine<const goal_lineage*> iterate() const = 0;
    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
};

#endif
