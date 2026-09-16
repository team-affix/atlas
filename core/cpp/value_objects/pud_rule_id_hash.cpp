#include "value_objects/pud_rule_id_hash.hpp"

#include <functional>
#include <variant>

size_t pud_rule_id_hash::operator()(const pud_rule_id& id) const noexcept {
    if (const pud_rule_id::axiom* axiom = std::get_if<pud_rule_id::axiom>(&id.content)) {
        size_t seed = std::hash<size_t>{}(axiom->entry_idx);
        return hash_combine(seed, 1);
    }

    const pud_rule_id::inference& inference = std::get<pud_rule_id::inference>(id.content);
    size_t seed = std::hash<const pud_rule_id*>{}(inference.caller);
    seed = hash_combine(seed, std::hash<size_t>{}(inference.call_site));
    seed = hash_combine(seed, std::hash<const pud_rule_id*>{}(inference.callee));
    return hash_combine(seed, 2);
}

size_t pud_rule_id_hash::hash_combine(size_t seed, size_t value) noexcept {
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
