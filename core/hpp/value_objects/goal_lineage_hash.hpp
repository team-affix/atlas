#ifndef GOAL_LINEAGE_HASH_HPP
#define GOAL_LINEAGE_HASH_HPP

#include <cstddef>
#include "lineage.hpp"

struct goal_lineage_hash {
    size_t operator()(const goal_lineage& lineage) const noexcept;
};

#endif
