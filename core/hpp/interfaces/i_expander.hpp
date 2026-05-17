#ifndef I_EXPANDER_HPP
#define I_EXPANDER_HPP

#include "../utility/state_machine.hpp"
#include "../value_objects/goal.hpp"
#include "../value_objects/candidate.hpp"
#include "../value_objects/rule.hpp"

struct i_expander {
    virtual ~i_expander() = default;
    virtual state_machine<goal> expand(const goal&, const candidate&, size_t) = 0;
};

#endif
