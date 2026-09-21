#ifndef PUD_DB_NODE_HPP
#define PUD_DB_NODE_HPP

#include <compare>
#include <cstdint>
#include <vector>
#include "pud_added_unification.hpp"
#include "expr.hpp"

struct pud_db_node {
    std::vector<pud_added_unification> added_unifications;
    std::vector<const expr*> added_body_goals;
    uint32_t lvc;
    std::strong_ordering operator<=>(const pud_db_node& other) const;
    bool operator==(const pud_db_node& other) const;
};

#endif
