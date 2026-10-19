#ifndef PUD_DERIVATION_ID_HASH_HPP
#define PUD_DERIVATION_ID_HASH_HPP

#include <cstddef>
#include "value_objects/pud_derivation_id.hpp"

struct pud_derivation_id_hash {
    size_t operator()(const pud_derivation_id& derivation) const noexcept;
};

#endif
