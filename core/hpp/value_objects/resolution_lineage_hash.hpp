#ifndef RESOLUTION_LINEAGE_HASH_HPP
#define RESOLUTION_LINEAGE_HASH_HPP

#include <cstddef>
#include "lineage.hpp"

struct resolution_lineage_hash {
    size_t operator()(const resolution_lineage& lineage) const noexcept;
};

#endif
