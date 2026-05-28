#ifndef I_GET_EXPR_COUNT_HPP
#define I_GET_EXPR_COUNT_HPP

#include <cstddef>

struct i_get_expr_count {
    virtual ~i_get_expr_count() = default;
    virtual size_t size() const = 0;
};

#endif

