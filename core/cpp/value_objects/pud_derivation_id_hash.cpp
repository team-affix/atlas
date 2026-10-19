#include "value_objects/pud_derivation_id_hash.hpp"

#include <functional>

size_t pud_derivation_id_hash::operator()(const pud_derivation_id& derivation) const noexcept {
    return std::hash<uint32_t>{}(derivation.value);
}
