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

inline bind_map::bind_map(globalizer& g) : globalizer_(g) {}

inline void bind_map::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    auto [_, inserted] = bindings_.insert({global_key, value});
    DEBUG_ASSERT(inserted);
}

inline void bind_map::clear_bindings() {
    bindings_.clear();
}

inline framed_expr bind_map::whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    auto it = bindings_.find(global_key);
    if (it == bindings_.end())
        return fe;
    framed_expr resolved = whnf(it->second);
    it->second = resolved;
    return resolved;
}

#endif
