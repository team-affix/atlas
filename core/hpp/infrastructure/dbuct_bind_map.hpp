#ifndef DBUCT_BIND_MAP_HPP
#define DBUCT_BIND_MAP_HPP

#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_assign.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/globalizer.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of bind_map.
//
// Every mutation is journalled on the trail (supplied as the abstract
// ILogTrailAction, not a concrete trail) — there is no conditional/unjournalled
// mode. bind() logs a backtrackable map insert so any substitution made since a
// choice frame is reverted exactly when the trail pops.
//
// whnf keeps the union-find path-compression write-back (collapsing a resolved
// chain to depth 1, so repeated lookups stay amortised O(1) instead of degrading
// to O(chain length)), made trail-safe by logging the compression write as a
// backtrackable map assign rather than applying it silently. A compressed value
// depends only on bindings at or below the current frame, and the compression is
// always logged at the current (top) frame, so on rollback it is undone before
// any binding it relies on — restoring the original chain link exactly. The write
// only fires when it actually shortens the chain, so an already-collapsed lookup
// logs nothing.
//
// The MHU's per-head local bind maps are also of this type and also always
// journal. Their tentative-unification transience is handled by the caller
// (a trail savepoint that pops on failure / squashes on success) rather than by
// suppressing journaling here, so this class stays a single uniform mode.
template<typename ILogTrailAction>
struct dbuct_bind_map {
    using map_t = std::unordered_map<uint32_t, framed_expr>;

    dbuct_bind_map(globalizer& g, ILogTrailAction& t);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

private:
    globalizer& globalizer_;
    ILogTrailAction& trail_;
    map_t bindings_;
};

template<typename ILogTrailAction>
dbuct_bind_map<ILogTrailAction>::dbuct_bind_map(globalizer& g, ILogTrailAction& t)
    : globalizer_(g), trail_(t) {}

template<typename ILogTrailAction>
void dbuct_bind_map<ILogTrailAction>::bind(uint32_t global_key, framed_expr value) {
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

template<typename ILogTrailAction>
framed_expr dbuct_bind_map<ILogTrailAction>::whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    auto it = bindings_.find(global_key);
    if (it == bindings_.end())
        return fe;
    const framed_expr resolved = whnf(it->second);
    // Path compression. whnf never inserts (only bind() does), so the recursion
    // above cannot rehash bindings_ and `it` stays valid. Skip the write when the
    // link is already collapsed so no-op compressions never touch the trail.
    if (!(it->second == resolved)) {
        auto m = std::make_unique<backtrackable_map_assign<map_t>>(global_key, resolved);
        m->capture(bindings_);
        m->invoke();
        trail_.log(std::move(m));
    }
    return resolved;
}

#endif
