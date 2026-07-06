#ifndef DBUCT_BIND_MAP_HPP
#define DBUCT_BIND_MAP_HPP

#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of bind_map.
//
// Bindings are journalled on the shared trail: bind() logs a backtrackable map
// insert so any substitution made since a choice frame is reverted exactly when
// the trail pops. whnf performs NO path-compression write-back (unlike the
// restarting bind_map): a compression write is a hidden mutation that the trail
// would otherwise have to journal, so it is dropped (correctness is unaffected;
// only intermediate sharing is lost).
//
// A journaling toggle supports the MHU: each head owns a heap-allocated local
// bind_map that is mutated during a tentative unification BEFORE the head is
// committed. Those creation-time binds run with journaling off (they are
// subsumed by the head arena's own undo, which destroys the whole local map on
// backtrack); once the head commits, enable_journaling() makes subsequent rebase
// binds individually reversible. The common bind_map is constructed with
// journaling on.
struct dbuct_bind_map {
    using map_t = std::unordered_map<uint32_t, framed_expr>;

    dbuct_bind_map(globalizer& g, trail& t, bool journaling);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

    void enable_journaling();

private:
    globalizer& globalizer_;
    trail& trail_;
    bool journaling_;
    map_t bindings_;
};

inline dbuct_bind_map::dbuct_bind_map(globalizer& g, trail& t, bool journaling)
    : globalizer_(g), trail_(t), journaling_(journaling) {}

inline void dbuct_bind_map::enable_journaling() { journaling_ = true; }

inline void dbuct_bind_map::bind(uint32_t global_key, framed_expr value) {
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

inline framed_expr dbuct_bind_map::whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    auto it = bindings_.find(global_key);
    if (it == bindings_.end())
        return fe;
    return whnf(it->second);
}

#endif
