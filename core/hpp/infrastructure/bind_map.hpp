#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <unordered_map>
#include "infrastructure/globalizer.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

struct bind_map {
    explicit bind_map(globalizer&);
    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr);
    void clear_bindings();
private:
    globalizer& globalizer_;
    std::unordered_map<uint32_t, framed_expr> bindings_;
};

#endif
