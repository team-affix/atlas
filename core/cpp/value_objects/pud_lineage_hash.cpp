#include "value_objects/pud_lineage_hash.hpp"

#include <functional>
#include <variant>

size_t pud_lineage_hash::operator()(const pud_lineage& lineage) const noexcept {
    if (const pud_lineage::axiom* axiom = std::get_if<pud_lineage::axiom>(&lineage.content)) {
        size_t seed = std::hash<size_t>{}(axiom->entry_idx);
        return hash_combine(seed, 1);
    }

    const pud_lineage::inference& inference = std::get<pud_lineage::inference>(lineage.content);
    size_t seed = std::hash<const pud_lineage*>{}(inference.caller);
    seed = hash_combine(seed, std::hash<size_t>{}(inference.call_site));
    seed = hash_combine(seed, std::hash<const pud_lineage*>{}(inference.callee));
    return hash_combine(seed, 2);
}

size_t pud_lineage_hash::hash_combine(size_t seed, size_t value) noexcept {
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
