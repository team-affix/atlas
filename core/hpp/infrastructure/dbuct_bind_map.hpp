#ifndef DBUCT_BIND_MAP_HPP
#define DBUCT_BIND_MAP_HPP

#include <unordered_map>
#include "infrastructure/globalizer.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of bind_map.
//
// Copyable (so MHU unify-heads that own a local bind_map can be deep-cloned for
// checkpointing) and snapshot/restore capable so the common substitution store
// rolls back exactly to any choice boundary. Because a snapshot captures the
// entire bindings map, the whnf path-compression write-back is completely safe
// here: any compression performed since a checkpoint is reverted by restore(),
// so the classic "compressed a binding that a shallower frame must not see"
// hazard cannot occur.
struct dbuct_bind_map {
    using snapshot_t = std::unordered_map<uint32_t, framed_expr>;

    explicit dbuct_bind_map(globalizer& g);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    globalizer& globalizer_;
    std::unordered_map<uint32_t, framed_expr> bindings_;
};

inline dbuct_bind_map::dbuct_bind_map(globalizer& g) : globalizer_(g) {}

inline void dbuct_bind_map::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    auto [_, inserted] = bindings_.insert({global_key, value});
    DEBUG_ASSERT(inserted);
}

inline framed_expr dbuct_bind_map::whnf(framed_expr fe) {
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

inline dbuct_bind_map::snapshot_t dbuct_bind_map::snapshot() const { return bindings_; }
inline void dbuct_bind_map::restore(snapshot_t s) { bindings_ = std::move(s); }

#endif
