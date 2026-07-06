#ifndef DBUCT_BIND_MAP_HPP
#define DBUCT_BIND_MAP_HPP

#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_assign.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of bind_map. Every mutation is journalled on the
// trail (via ILogTrailAction) — there is no unjournalled mode: bind() logs an
// insert and whnf's path-compression write logs an assign, so both rewind exactly
// on trail pop.
template<typename IGlobalize, typename ILogTrailAction>
struct dbuct_bind_map {
    using map_t = std::unordered_map<uint32_t, framed_expr>;

    dbuct_bind_map(IGlobalize& g, ILogTrailAction& t);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

private:
    IGlobalize& globalizer_;
    ILogTrailAction& trail_;
    map_t bindings_;
};

template<typename IGlobalize, typename ILogTrailAction>
dbuct_bind_map<IGlobalize, ILogTrailAction>::dbuct_bind_map(IGlobalize& g, ILogTrailAction& t)
    : globalizer_(g), trail_(t) {}

template<typename IGlobalize, typename ILogTrailAction>
void dbuct_bind_map<IGlobalize, ILogTrailAction>::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    auto m = std::make_unique<backtrackable_map_insert<map_t>>(global_key, value);
    m->capture(bindings_);
    m->invoke();
    trail_.log(std::move(m));
}

template<typename IGlobalize, typename ILogTrailAction>
framed_expr dbuct_bind_map<IGlobalize, ILogTrailAction>::whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    auto it = bindings_.find(global_key);
    if (it == bindings_.end())
        return fe;
    const framed_expr resolved = whnf(it->second);
    if (!(it->second == resolved)) {
        auto m = std::make_unique<backtrackable_map_assign<map_t>>(global_key, resolved);
        m->capture(bindings_);
        m->invoke();
        trail_.log(std::move(m));
    }
    return resolved;
}

#endif
