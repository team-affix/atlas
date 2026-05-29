#ifndef I_RULE_ID_SET_HPP
#define I_RULE_ID_SET_HPP

#include <cstddef>
#include <memory>
#include "value_objects/lineage.hpp"
#include "infrastructure/coroutine.hpp"

struct i_rule_id_set {
    virtual ~i_rule_id_set() = default;
    virtual void insert(rule_id) = 0;
    virtual void erase(rule_id) = 0;
    virtual coroutine<rule_id, void> iterate() const = 0;
    virtual size_t size() const = 0;
    virtual std::unique_ptr<i_rule_id_set> copy() const = 0;
};

#endif
