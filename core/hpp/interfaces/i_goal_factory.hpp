#ifndef I_GOAL_FACTORY_HPP
#define I_GOAL_FACTORY_HPP

#include "../interfaces/i_factory.hpp"
#include "../value_objects/goal.hpp"
#include "../value_objects/rule.hpp"

struct i_goal_expander : i_factory<goal, size_t> {
    virtual ~i_goal_factory() = default;
    virtual void seed_expansion(const goal&, const rule&) = 0;
};

#endif
