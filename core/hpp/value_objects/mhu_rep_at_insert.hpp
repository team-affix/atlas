#ifndef MHU_REP_AT_INSERT_HPP
#define MHU_REP_AT_INSERT_HPP

#include <compare>
#include <cstdint>
#include "value_objects/lineage.hpp"

struct mhu_rep_at_insert {
    uint32_t rep;
    const resolution_lineage* rl;
    auto operator<=>(const mhu_rep_at_insert&) const = default;
};

#endif
