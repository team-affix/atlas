#ifndef I_RULE_SET_HPP
#define I_RULE_SET_HPP

#include "../value_objects/rule.hpp"
#include "../utility/state_machine.hpp"

struct i_rule_set {
    virtual ~i_rule_set() = default;
    virtual void insert(const rule*) = 0;
    virtual void erase(const rule*) = 0;
    virtual state_machine<const rule*> iterate() const = 0;
    virtual size_t size() const = 0;
};

#endif
