#ifndef NEAREST_DECISION_INSERT_HPP
#define NEAREST_DECISION_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct nearest_decision_insert {
    const resolution_lineage* key;
    const resolution_lineage* value;
    auto operator<=>(const nearest_decision_insert&) const = default;
};

#endif
