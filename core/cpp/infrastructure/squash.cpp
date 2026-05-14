#include "../../hpp/infrastructure/squash.hpp"

squash::squash() {
}

void squash::bind(uint32_t index, const expr* e) {
    bindings[index] = e;
}

const expr* squash::whnf(const expr* key) {
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

    // Collapse the binding
    bind(var.index, whnf_bound_value);

    return whnf_bound_value;
}
