#ifndef HIERARCHICAL_BIND_MAP_HPP
#define HIERARCHICAL_BIND_MAP_HPP

#include <cstdint>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"
#include "debug_assert.hpp"

template<typename IGlobalize, typename ITransient>
struct hierarchical_bind_map {
    hierarchical_bind_map(IGlobalize& g, ITransient& transient);
    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);
private:
    IGlobalize& globalizer_;
    ITransient& transient_;
};

template<typename IGlobalize, typename ITransient>
hierarchical_bind_map<IGlobalize, ITransient>::hierarchical_bind_map(IGlobalize& g, ITransient& transient)
    : globalizer_(g)
    , transient_(transient) {}

template<typename IGlobalize, typename ITransient>
void hierarchical_bind_map<IGlobalize, ITransient>::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    DEBUG_ASSERT(!transient_.find(global_key));
    transient_.set(global_key, value);
}

template<typename IGlobalize, typename ITransient>
framed_expr hierarchical_bind_map<IGlobalize, ITransient>::whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    const framed_expr* found = transient_.find(global_key);
    if (!found)
        return fe;
    framed_expr resolved = whnf(*found);
    transient_.set(global_key, resolved);
    return resolved;
}

#endif
