#ifndef I_GET_DECISION_COUNT_HPP
#define I_GET_DECISION_COUNT_HPP

#include <cstddef>

struct i_get_decision_count {
    virtual ~i_get_decision_count() = default;
    virtual size_t get_decision_count() const = 0;
};

#endif

