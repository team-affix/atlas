#include "debug_assert.hpp"
#include "infrastructure/bind_map.hpp"

bind_map::bind_map() {
}

void bind_map::bind(uint32_t index, const expr* e) {
    // make sure we bind younger to older
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(e->content)
        || index > std::get<expr::var>(e->content).index);
    auto [_, inserted] = bindings.insert({index, e});
    // make sure we actually bound
    DEBUG_ASSERT(inserted);
}

void bind_map::clear_bindings() {
    bindings.clear();
}

const expr* bind_map::whnf(const expr* key) {
    // If the key is not a variable, it is already in WHNF
    if (!std::holds_alternative<expr::var>(key->content))
        return key;

    // Get the variable out of the key
    const expr::var& var = std::get<expr::var>(key->content);

    // Check if the variable is bound
    auto it = bindings.find(var.index);
    
    // If the variable is not bound, return the key
    if (it == bindings.end())
        return key;

    // Get the bound value
    const expr* bound_value = it->second;
        
    // WHNF the bound value
    const expr* whnf_bound_value = whnf(bound_value);

    // Path compression — update existing entry only (not public bind)
    bindings[var.index] = whnf_bound_value;

    return whnf_bound_value;
}
