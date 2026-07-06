#ifndef BIND_MAP_HPP
#define BIND_MAP_HPP

#include <unordered_map>
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<typename IGlobalize>
struct bind_map {
    explicit bind_map(IGlobalize&);
    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr);
    void clear_bindings();
private:
    IGlobalize& globalizer_;
    std::unordered_map<uint32_t, framed_expr> bindings_;
};

template<typename IGlobalize>
bind_map<IGlobalize>::bind_map(IGlobalize& g) : globalizer_(g) {}

template<typename IGlobalize>
void bind_map<IGlobalize>::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    auto [_, inserted] = bindings_.insert({global_key, value});
    DEBUG_ASSERT(inserted);
}

template<typename IGlobalize>
void bind_map<IGlobalize>::clear_bindings() {
    bindings_.clear();
}

template<typename IGlobalize>
framed_expr bind_map<IGlobalize>::whnf(framed_expr fe) {
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
