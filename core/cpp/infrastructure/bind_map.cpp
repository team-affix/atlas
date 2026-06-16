#include "debug_assert.hpp"
#include "infrastructure/bind_map.hpp"

bind_map::bind_map() {}

void bind_map::bind(uint32_t global_key, framed_expr value) {
    // Bind younger (higher global key) to older.
    // If value is a var, its global key must be strictly less than global_key.
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > value.frame_offset + std::get<expr::var>(value.skeleton->content).index);
    auto [_, inserted] = bindings_.insert({global_key, value});
    DEBUG_ASSERT(inserted);
}

void bind_map::clear_bindings() {
    bindings_.clear();
}

framed_expr bind_map::whnf(framed_expr fe) {
    // Functors are already in WHNF.
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;

    const uint32_t global_key = fe.frame_offset + std::get<expr::var>(fe.skeleton->content).index;

    auto it = bindings_.find(global_key);
    if (it == bindings_.end())
        return fe;

    // Follow chain and path-compress.
    framed_expr resolved = whnf(it->second);
    it->second = resolved;
    return resolved;
}
