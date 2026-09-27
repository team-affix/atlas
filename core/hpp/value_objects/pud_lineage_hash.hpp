#ifndef PUD_LINEAGE_HASH_HPP
#define PUD_LINEAGE_HASH_HPP

#include <cstddef>
#include "pud_lineage.hpp"

struct pud_lineage_hash {
    size_t operator()(const pud_lineage& lineage) const noexcept;

private:
    static size_t hash_combine(size_t seed, size_t value) noexcept;
};

#endif
