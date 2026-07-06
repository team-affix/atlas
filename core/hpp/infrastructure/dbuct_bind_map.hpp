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
// Bindings are journalled on the trail (supplied as the abstract ILogTrailAction,
// not a concrete trail): bind() logs a backtrackable map insert so any
// substitution made since a choice frame is reverted exactly when the trail pops.
//
// whnf keeps the union-find path-compression write-back (collapsing a resolved
// chain to depth 1, so repeated lookups stay amortised O(1) instead of degrading
// to O(chain length)), but makes it trail-safe: when journalling, the compression
// write is logged as a backtrackable map assign rather than applied silently. A
// compressed value depends only on bindings at or below the current frame, and
// the compression is always logged at the current (top) frame, so on rollback it
// is undone before any binding it relies on — restoring the original chain link
// exactly. The write only fires when it actually shortens the chain, so an
// already-collapsed lookup logs nothing.
//
// A journaling toggle supports the MHU: each head owns a heap-allocated local
// bind_map that is mutated during a tentative unification BEFORE the head is
// committed. Those creation-time binds run with journaling off (they are
// subsumed by the head arena's own undo, which destroys the whole local map on
// backtrack); once the head commits, enable_journaling() makes subsequent rebase
// binds individually reversible. The common bind_map is constructed with
// journaling on.
template<typename ILogTrailAction>
struct dbuct_bind_map {
    using map_t = std::unordered_map<uint32_t, framed_expr>;

    dbuct_bind_map(globalizer& g, ILogTrailAction& t, bool journaling);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

    void enable_journaling();

private:
    globalizer& globalizer_;
    ILogTrailAction& trail_;
    bool journaling_;
    map_t bindings_;
};

template<typename ILogTrailAction>
dbuct_bind_map<ILogTrailAction>::dbuct_bind_map(globalizer& g, ILogTrailAction& t, bool journaling)
    : globalizer_(g), trail_(t), journaling_(journaling) {}

template<typename ILogTrailAction>
void dbuct_bind_map<ILogTrailAction>::enable_journaling() { journaling_ = true; }

template<typename ILogTrailAction>
void dbuct_bind_map<ILogTrailAction>::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    if (journaling_) {
        auto m = std::make_unique<backtrackable_map_insert<map_t>>(global_key, value);
        m->capture(bindings_);
        m->invoke();
        trail_.log(std::move(m));
    } else {
        auto [_, inserted] = bindings_.insert({global_key, value});
        DEBUG_ASSERT(inserted);
    }
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
        if (journaling_) {
            auto m = std::make_unique<backtrackable_map_assign<map_t>>(global_key, resolved);
            m->capture(bindings_);
            m->invoke();
            trail_.log(std::move(m));
        } else {
            it->second = resolved;
        }
    }
    return resolved;
}

#endif
