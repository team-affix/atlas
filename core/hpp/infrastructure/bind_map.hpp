#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <unordered_map>
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_clear_bindings.hpp"
#include "interfaces/i_globalizer.hpp"
#include "value_objects/framed_expr.hpp"

struct bind_map
    : i_bind_map
    , i_clear_bindings {
    virtual ~bind_map() = default;
    explicit bind_map(i_globalizer&);
    void bind(uint32_t global_key, framed_expr value) override;
    framed_expr whnf(framed_expr) override;
    void clear_bindings() override;
private:
    i_globalizer& globalizer_;
    std::unordered_map<uint32_t, framed_expr> bindings_;
};

#endif
